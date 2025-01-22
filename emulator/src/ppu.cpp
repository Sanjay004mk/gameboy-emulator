#include "emupch.h"

#include "cpu.h"
#include "rom.h"
#include "ppu.h"

namespace emu
{
	PPU::PPU(const Memory& memory)
		: memory(memory)
	{
		rdr::BufferConfiguration bufferConfig;
		bufferConfig.cpuVisible = true;
		bufferConfig.enableCopy = true;
		bufferConfig.persistentMap = true;
		bufferConfig.size = 256 * 256 * 4;
		bufferConfig.type = rdr::BufferType::StagingBuffer;

		stagingBuffer = new rdr::Buffer(bufferConfig);
		cpuBuffer.resize(256 * 256);

		rdr::TextureConfiguration textureConfig;
		textureConfig.format = rdr::TextureFormat(4, rdr::eDataType::Ufloat8);
		textureConfig.size = { 256, 256 };
		textureConfig.type.Set<rdr::TextureType::copyEnable>(true);

		displayTexture = new rdr::Texture(textureConfig);

		memset(cpuBuffer.data(), 0xff, cpuBuffer.size() * 4);
		memcpy_s(stagingBuffer->GetData(), stagingBuffer->GetConfig().size, cpuBuffer.data(), cpuBuffer.size() * 4);
		displayTexture->SetData(stagingBuffer);
	}

	PPU::~PPU()
	{
		delete stagingBuffer;
		delete displayTexture;
	}

	void PPU::step(uint32_t cycles)
	{
		if ((cycAcc += cycles) >= CPU::frequency / 60)
		{
			cycAcc -= CPU::frequency / 60;
			SetTextureData();
		}
	}

	void PPU::SetTextureData()
	{
		uint32_t bgTileData = memory[0xff40] & (1 << 4) ? 0x8000 : 0x8800;
		uint32_t bgTileMap = memory[0xff40] & (1 << 3) ? 0x9c00 : 0x9800;
		uint32_t scx = memory[0xff43];
		uint32_t scy = memory[0xff42];

		// tmp
		uint32_t color_palette[] = {
			0xffffffff,
			0xff606060,
			0xff303030,
			0xff010101
		};

		for (uint32_t i = 0; i < 256; i++)
		{
			for (uint32_t j = 0; j < 256; j++)
			{
				uint32_t tileNumber = memory[bgTileMap + ((i / 8) * 4) + (j / 8)]; // 32 x 32 byte tilemap -> 8 x 8 pixels per tile

				//  every tile is represented by 16 bytes;
				//  every 2 consecutive bytes represents a row (8 pixels)
				//  the palette to use (0 - 3) is given by taking the corresponding bits from the 2 bytes
				//  eg. for 2nd pixel from the left take bit 6 from both bytes and flip them
				//  0x8800 represents signed indexing

				uint32_t location =
					bgTileData == 0x8000 ?
					bgTileData + (tileNumber * 0x10) + ((i % 8) * 2) :
					bgTileData + ((int8_t)tileNumber * 0x10) + ((i % 8) * 2); // signed indexing

				uint16_t tileData = memory.As<uint16_t>(location); // tile row

				uint32_t index = 0;
				index = ((tileData << (j % 8)) & (0x8000)) ? 0x1 : 0x0;
				index |= ((tileData << (j & 8)) & (0x0080)) ? 0x2 : 0x0;

				RDR_ASSERT_MSG_BREAK(index < 4, "Index wrong");
				cpuBuffer[i * 256 + j] = color_palette[index];
			}
		}

		memcpy_s(stagingBuffer->GetData(), stagingBuffer->GetConfig().size, cpuBuffer.data(), cpuBuffer.size() * 4);
		displayTexture->SetData(stagingBuffer, false);
	}
}