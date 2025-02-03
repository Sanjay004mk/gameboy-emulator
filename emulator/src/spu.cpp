#include "emupch.h"

#define MINIAUDIO_IMPLEMENTATION
#include "spu.h"

#include <renderer/time.h>

namespace utils
{
	static float phase = 0.f;
	static constexpr float frequency = 440.f;
	static constexpr float increment = 1.f / emu::SPU::SAMPLE_RATE;
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
		start = rdr::Time::GetTime();
	}

	SPU::~SPU()
	{
		ma_device_uninit(&audioDevice);
	}

	void SPU::step(uint32_t cycles)
	{
		
	}

	void SPU::Sample()
	{
		float val = glm::sin(glm::pi<float>() * 2.f * utils::frequency * utils::phase);

		if (queue.size_approx() < utils::queue_limit)
		{
			queue.enqueue(val);
			queue.enqueue(val);
		}

		utils::phase += utils::increment;

		if (utils::phase > 1.f)
			utils::phase -= 1.f;
	}

	SC::SC(Memory& memory)
		: memory(memory)
	{

	}

	SC1::SC1(Memory& memory)
		: SC(memory)
	{

	}

	SC2::SC2(Memory& memory)
		: SC(memory)
	{

	}

	SC3::SC3(Memory& memory)
		: SC(memory)
	{

	}

	SC4::SC4(Memory& memory)
		: SC(memory)
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