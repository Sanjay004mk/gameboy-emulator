#include "emupch.h"

#include <renderer/time.h>

#define MINIAUDIO_IMPLEMENTATION
#include "spu.h"

#include "cpu.h"

namespace utils
{
	static float phase = 0.f;
	static constexpr float frequency = 440.f;
	static constexpr float increment = 1.f / emu::SPU::SAMPLE_RATE;
	static constexpr size_t queue_limit = 4096;
	static std::chrono::steady_clock::time_point prev, cur;
	static float diff = 0.f;

	static void device_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount)
	{
		cur = std::chrono::high_resolution_clock::now();

		diff = std::chrono::duration<float>(cur - prev).count();

		emu::SPU* spu = (emu::SPU*)device->pUserData;
		auto& queue = spu->GetQueue();
		float* out = (float*)output;
		for (uint32_t i = 0; i < frameCount; i++)
		{
			queue.try_dequeue<float>(*out);
			queue.try_dequeue<float>(*(out + 1));

			out += 2;
		}
		prev = cur;
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
	}

	SPU::~SPU()
	{
		ma_device_uninit(&audioDevice);
	}

	void SPU::step(uint32_t cycles)
	{
		cycAcc += cycles;
		if (cycAcc > (CPU::frequency / SAMPLE_RATE))
		{
			cycAcc -= (CPU::frequency / SAMPLE_RATE);
			Sample();
		}
	}

	void SPU::Sample()
	{
		float val = glm::sin(glm::pi<float>() * 2.f * utils::frequency * utils::phase);

		if (queue.size_approx() < utils::queue_limit)
		{
			queue.enqueue(val);
			queue.enqueue(val);
		}
		else
			__debugbreak();

		utils::phase += utils::increment;

		if (utils::phase > 1.f)
			utils::phase -= 1.f;
	}

	void SPU::WriteCallback(uint32_t address, uint8_t value)
	{

	}

	SC::SC(Memory& memory, uint32_t nrOffset)
		: memory(memory), nrOffset(nrOffset)
	{

	}

	uint8_t& SC::nrx0()	{ return memory.memory[0xff10 + nrOffset + 0x0]; }
	uint8_t& SC::nrx1() { return memory.memory[0xff10 + nrOffset + 0x1]; }
	uint8_t& SC::nrx2() { return memory.memory[0xff10 + nrOffset + 0x2]; }
	uint8_t& SC::nrx3() { return memory.memory[0xff10 + nrOffset + 0x3]; }
	uint8_t& SC::nrx4() { return memory.memory[0xff10 + nrOffset + 0x4]; }

	uint32_t SC::GetFrequency()	{ return (uint32_t)nrx3() | ((uint32_t)(nrx4() & 7) << 8); }
	
	uint32_t SC::GetVoulme() { return ((uint32_t)nrx2() & 0xf0) >> 4; }
	bool SC::IsVolumeAdd() { return nrx2() & 0x08; }
	uint32_t SC::GetVolumeSweepPeriod() { return nrx2() & 0x07; }
	
	uint32_t SC::GetLengthCounter() { return 64 - (nrx1() & 0x3f); }

	uint32_t SC::GetDutyIndex() { return (nrx1() & 0xc0) >> 6; }
	uint32_t SC::GetCurrentDuty() { return utils::dutyCycles[GetDutyIndex()][frameIndex]; }

	void SC::SetFrequency(uint32_t frequency)
	{
		nrx3() = frequency & 0xff;
		nrx4() &= ~(0x7);
		nrx4() |= (frequency >> 8);
	}

	SC1::SC1(Memory& memory)
		: SC(memory, 0)
	{

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

	void SC1::step(uint32_t cycles)
	{
	}

	void SC2::step(uint32_t cycles)
	{
	}

	void SC3::step(uint32_t cycles)
	{
	}

	void SC4::step(uint32_t cycles)
	{
	}
}