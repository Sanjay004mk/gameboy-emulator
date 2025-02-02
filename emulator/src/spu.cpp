#include "emupch.h"

#define MINIAUDIO_IMPLEMENTATION
#include "spu.h"

namespace utils
{
	static float phase = 0.f;
	static constexpr float frequency = 440.f;
	static constexpr float increment = 1.f / 48000.f;

	static void device_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount)
	{
		std::vector<float> data(frameCount * 2);
		for (size_t i = 0; i < data.size(); i += 2)
		{
			data[i] = glm::sin(glm::pi<float>() * 2.f * phase * frequency);
			data[i + 1] = data[i];
			phase += increment;

			if (phase >= 1.f)
				phase -= 1.f;
		}

		memcpy(output, data.data(), frameCount * 2 * 4);
	}
}

namespace emu
{
	SPU::SPU(Memory& memory)
		: memory(memory)
	{
		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_f32;   
		config.playback.channels = 2;             
		config.sampleRate = 48000;           
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

	}
}