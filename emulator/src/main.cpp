#include <emupch.h>
#include <renderer/rdr.h>

static unsigned long x = 123456789, y = 362436069, z = 521288629;

unsigned long xorshf96(void) {          //period 2^96-1
	unsigned long t;
	x ^= x << 16;
	x ^= x >> 5;
	x ^= x << 1;

	t = x;
	x = y;
	y = z;
	z = t ^ x ^ y;

	return z;
}

int main(int argc, const char** argv) 
{
	rdr::Renderer::Init(argc, argv, "emulator");

	{
		rdr::WindowConfiguration config{};
		config.title = "Gameboy Emulator";

		auto window = rdr::Renderer::InstantiateWindow(config);

		window->RegisterCallback<rdr::KeyPressedEvent>([&](rdr::KeyPressedEvent& e)
			{
				if (window->IsKeyDown(rdr::Key::LeftControl))
				{
					switch (e.GetKeyCode())
					{
					case rdr::Key::D1:
						RDR_LOG_INFO("Setting Present Mode to No vsync");
						window->SetPresentMode(rdr::PresentMode::NoVSync);
						break;
					case rdr::Key::D2:
						RDR_LOG_INFO("Setting Present Mode to Triple buffered");
						window->SetPresentMode(rdr::PresentMode::TripleBuffer);
						break;
					case rdr::Key::D3:
						RDR_LOG_INFO("Setting Present Mode to Vsync");
						window->SetPresentMode(rdr::PresentMode::VSync);
						break;
					}
				}
			});

		rdr::TextureConfiguration textureConfig;
		textureConfig.format = rdr::TextureFormat(4, rdr::eDataType::Ufloat8);
		textureConfig.size = window->GetConfig().size;
		rdr::Texture texture(textureConfig);

		rdr::BufferConfiguration bufferConfig;
		bufferConfig.enableCopy = true;
		bufferConfig.persistentMap = true;
		bufferConfig.type = rdr::BufferType::StagingBuffer;
		bufferConfig.size = (uint32_t)texture.GetByteSize();
		rdr::Buffer buffer(bufferConfig);
		uint32_t* data = (uint32_t*)malloc(bufferConfig.size);

		rdr::TextureBlitInformation blitInfo;
		blitInfo.srcMax = texture.GetConfig().size;
		blitInfo.dstMax = window->GetConfig().size;

		RDR_LOG_INFO("Starting Main Loop");
		while (!window->ShouldClose())
		{
			rdr::Renderer::BeginFrame(window);

			for (uint32_t x = 0; x < textureConfig.size.x; x++)
				for (uint32_t y = 0; y < textureConfig.size.y; y++)
					data[x * textureConfig.size.y + y] = xorshf96();

			memcpy(buffer.GetData(), data, bufferConfig.size);

			texture.SetData(&buffer, false);

			rdr::Renderer::BlitToWindow(&texture, blitInfo);

			rdr::Renderer::EndFrame();

			rdr::Renderer::PollEvents();
		}
	}

	rdr::Renderer::Shutdown();
	return 0;
}