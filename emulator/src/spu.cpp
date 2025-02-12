#include "emupch.h"

#include <renderer/time.h>

#define MINIAUDIO_IMPLEMENTATION
#include "spu.h"

#include "cpu.h"

namespace utils
{
	static constexpr size_t queue_limit = 4096;

	static void device_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount)
	{
		emu::SPU* spu = (emu::SPU*)device->pUserData;
		auto& queue = spu->GetQueue();
		float* out = (float*)output;
		for (uint32_t i = 0; i < frameCount; i++)
		{
			queue.try_dequeue<float>(*out);
			queue.try_dequeue<float>(*(out + 1));

			out += 2;
		}
	}

	static uint8_t dutyCycles[][8] = {
		{ 0, 0, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 1, 1, 1 },
		{ 0, 1, 1, 1, 1, 1, 1, 0 }
	};
}

namespace emu
{
	SPU::SPU(Memory& memory)
		: memory(memory), queue(4096)
	{
		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_f32;   
		config.playback.channels = 2;             
		config.sampleRate = SAMPLE_RATE;           
		config.dataCallback = &utils::device_data_callback;   
		config.pUserData = this; 

		RDR_ASSERT_MSG((ma_device_init(NULL, &config, &audioDevice) == MA_SUCCESS), "Failed to initialize audio device");

		ma_device_start(&audioDevice);

		sc[0] = new SC1(memory);
		sc[1] = new SC2(memory);
		sc[2] = new SC3(memory);
		sc[3] = new SC4(memory);
	}

	SPU::~SPU()
	{
		for (auto& s : sc)
			delete s;

		ma_device_uninit(&audioDevice);
	}

	void SPU::step(uint32_t cycles)
	{
		for (auto& s : sc)
			s->step(cycles);

		cycAcc += cycles;
		if (cycAcc > (CPU::frequency / SAMPLE_RATE))
		{
			cycAcc -= (CPU::frequency / SAMPLE_RATE);
			Sample();
		}
	}

	void SPU::Sample()
	{
		if (queue.size_approx() < utils::queue_limit)
		{
			float val = 0.f;

			for (auto& s : sc)
				if (s->enabled)
					val += s->Sample();

			val /= 4.f;

			queue.enqueue(val);
			queue.enqueue(val);
		}
		else
			__debugbreak();
	}

	void SPU::WriteCallback(uint32_t address, uint8_t value)
	{
		memory.memory[address] = value;

		uint32_t channel = (address - 0xff10) / 5;
		uint32_t reg = (address - 0xff10) % 5;

		if (reg == 4 && (value & 0x80) && channel < 5)
			sc[channel]->Trigger();
	}

	SC::SC(Memory& memory, uint32_t nrOffset)
		: memory(memory), nrOffset(nrOffset)
	{

	}

	void SC::step(uint32_t cyc)
	{
		cycles += cyc;
		if (cycles >= 8192) // 512 hz
		{
			cycles -= 8192;
			frameIndex = (frameIndex + 1) % 8;
			FrameStep();
		}

		dutyTimer--;
		TimerTick();
	}

	void SC::TimerTick()
	{
		if (dutyTimer < 0)
		{
			dutyTimer = (2048 - GetFrequency()) * 4;
			dutyIndex = (dutyIndex + 1) % 8;
		}
	}

	void SC::LengthCounterStep()
	{
		if (GetLengthEnable())
			if (--lengthCounter <= 0)
				enabled = false;
	}

	void SC::VolumeEnvelopeStep()
	{
		if (!updateVolume || --volumeEnvelopePeriod > 0)
			return;

		volumeEnvelopePeriod = GetVolumeSweepPeriod();

		volumeEnvelope += IsVolumeAdd() ? 1 : -1;
		if (volumeEnvelope > 15 || volumeEnvelope < 0)
		{
			volumeEnvelope = glm::clamp(volumeEnvelope, 0, 15);
			updateVolume = false;
		}
	}

	uint8_t& SC::nrx0()	{ return memory.memory[0xff10 + nrOffset + 0x0]; }
	uint8_t& SC::nrx1() { return memory.memory[0xff10 + nrOffset + 0x1]; }
	uint8_t& SC::nrx2() { return memory.memory[0xff10 + nrOffset + 0x2]; }
	uint8_t& SC::nrx3() { return memory.memory[0xff10 + nrOffset + 0x3]; }
	uint8_t& SC::nrx4() { return memory.memory[0xff10 + nrOffset + 0x4]; }
	uint8_t& SC::nrx(uint32_t i)
	{
		switch (i)
		{
		case 1: return nrx1();
		case 2: return nrx2();
		case 3: return nrx3();
		case 4: return nrx4();
		default:	return nrx0();
		}
	}

	uint32_t SC::GetFrequency()	{ return (uint32_t)nrx3() | ((uint32_t)(nrx4() & 7) << 8); }
	
	uint32_t SC::GetVolume() { return ((uint32_t)nrx2() & 0xf0) >> 4; }
	bool SC::IsVolumeAdd() { return nrx2() & 0x08; }
	uint32_t SC::GetVolumeSweepPeriod() { return nrx2() & 0x07; }
	
	uint32_t SC::GetLengthCounter() { return 64 - (nrx1() & 0x3f); }
	bool SC::GetLengthEnable() { return nrx4() & 0x40; }

	uint32_t SC::GetDutyIndex() { return (nrx1() & 0xc0) >> 6; }
	uint32_t SC::GetCurrentDuty() { return utils::dutyCycles[GetDutyIndex()][frameIndex]; }

	void SC::SetFrequency(uint32_t frequency)
	{
		nrx3() = frequency & 0xff;
		nrx4() &= ~(0x7);
		nrx4() |= (frequency >> 8);
	}

	void SC::Trigger()
	{
		enabled = true;
		updateVolume = true;

		if (lengthCounter == 0)
			lengthCounter = 64 - GetLengthCounter();

		dutyTimer = (2048 - GetFrequency()) * 4;
		volumeEnvelopePeriod = GetVolumeSweepPeriod();
		volumeEnvelope = GetVolume();
		dutyIndex = 0;
	}

	SC1::SC1(Memory& memory)
		: SC(memory, 0)
	{

	}

	void SC1::FrequencySweepStep()
	{
		uint32_t period = nrx0() & 0x70 >> 4, shift = nrx0() & 0x07;

		sweepPeriod--;
		if (sweepPeriod > 0)
			return;
			
		sweepPeriod = period ? period : 8;
		enabled = period || shift;

		if (shift)
		{
			int32_t newFrequency = shadowFrequency >> shift;
			if (nrx0() & 0x10)
				newFrequency = -newFrequency;

			newFrequency += shadowFrequency;
			if (newFrequency > 2047)
				enabled = false;
			else
			{
				shadowFrequency = newFrequency;
				SetFrequency(newFrequency);

				if ((newFrequency + ((shadowFrequency >> shift) * ((nrx0() & 0x10) ? -1 : 1))) > 2047)
					enabled = false;
			}
		}
	}

	SC2::SC2(Memory& memory)
		: SC(memory, 5)
	{

	}

	SC3::SC3(Memory& memory)
		: SC(memory, 10)
	{

	}

	SC4::SC4(Memory& memory)
		: SC(memory, 15)
	{

	}

	void SC1::FrameStep()
	{
		if (frameIndex % 2 == 0)
			LengthCounterStep();

		if (frameIndex == 2 || frameIndex == 6)
			FrequencySweepStep();

		if (frameIndex == 7)
			VolumeEnvelopeStep();
	}

	void SC2::FrameStep()
	{
		if (frameIndex % 2 == 0)
			LengthCounterStep();

		if (frameIndex == 7)
			VolumeEnvelopeStep();
	}

	void SC3::FrameStep()
	{
		if (frameIndex % 2 == 0)
			LengthCounterStep();
	}

	void SC4::FrameStep()
	{
		if (frameIndex % 2 == 0)
			LengthCounterStep();

		if (frameIndex == 7)
			VolumeEnvelopeStep();
	}

	float SC1::Sample()
	{
		return GetCurrentDuty() * volumeEnvelope / 15.f;
	}

	float SC2::Sample()
	{
		return GetCurrentDuty() * volumeEnvelope / 15.f;
	}

	uint32_t SC3::GetVolume()
	{
		uint8_t c = (nrx2() & 0x60) >> 5;
		switch (c)
		{
		case 1: return 15;
		case 2: return 15 / 2;
		case 3: return 15 / 4;
		default: return 0;
		}
	}

	float SC3::Sample()
	{
		uint8_t wave = memory.memory[0xff30 + (dutyIndex / 2)];
		wave = dutyIndex & 1 ? wave & 0x0f : wave >> 4;
		return (wave * GetVolume()) / (0xf * 15.f);
	}

	float SC4::Sample()
	{
		return (lfsr & 0x01) * volumeEnvelope / 15.f;
	}

	void SC3::TimerTick()
	{
		if (dutyTimer < 0)
		{
			dutyTimer = (2048 - GetFrequency()) * 2;
			dutyIndex = (dutyIndex + 1) % 32;
		}
	}

	void SC4::TimerTick()
	{
		if (dutyTimer < 0)
		{
			dutyTimer = (nrx2() & 0x07) << ((nrx2() & 0xf0) >> 4);
			uint8_t newDuty = (lfsr & 0x01) ^ ((lfsr >> 1) & 0x01);
			lfsr >>= 1;
			lfsr |= (newDuty << 14);
			if (nrx2() & 0x08)
			{
				lfsr |= (newDuty << 6);
				lfsr &= 0x7f;
			}
		}
	}
}