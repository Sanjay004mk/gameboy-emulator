#include "emupch.h"

#include "cpu.h"
#include "rom.h"
#include "ppu.h"

// tmp
uint32_t color_palette[] = {
	0xffffffff,
	0xff606060,
	0xff303030,
	0xff010101
};

namespace emu
{
	PPU::TextureMap::TextureMap(uint32_t w, uint32_t h)
	{
		rdr::BufferConfiguration bufferConfig;
		bufferConfig.cpuVisible = true;
		bufferConfig.enableCopy = true;
		bufferConfig.persistentMap = true;
		bufferConfig.size = w * h * 4;
		bufferConfig.type = rdr::BufferType::StagingBuffer;

		stagingBuffer = new rdr::Buffer(bufferConfig);

		rdr::TextureConfiguration textureConfig;
		textureConfig.format = rdr::TextureFormat(4, rdr::eDataType::Ufloat8);
		textureConfig.size = { w, h };
		textureConfig.type.Set<rdr::TextureType::copyEnable>(true);
		textureConfig.type.Set<rdr::TextureType::sampled>(true);

		texture = new rdr::Texture(textureConfig);

		cpuBuffer.resize(w * h);

		memset(stagingBuffer->GetData(), 0xff, stagingBuffer->GetConfig().size);
		texture->SetData(stagingBuffer);
	}

	PPU::TextureMap::~TextureMap()
	{
		delete stagingBuffer;
		delete texture;
	}

	void PPU::TextureMap::LoadStagingBuffer()
	{
		memcpy_s(stagingBuffer->GetData(), stagingBuffer->GetConfig().size, cpuBuffer.data(), cpuBuffer.size() * 4);
	}

	void PPU::TextureMap::UpdateTexture(bool async)
	{
		texture->SetData(stagingBuffer, async);
	}

	PPU::PPU(Memory& memory)
		: memory(memory), display(160, 144), bg(256, 256), window(256, 256), sprite(256, 256), tileMap(256, 256), tiles(256, 256)
	{
		
	}

	PPU::~PPU()
	{
	}

	void PPU::step(uint32_t cycles)
	{
		cycAcc += cycles;

		uint32_t ly = memory[0xff44];
		uint32_t lyc = memory[0xff45];

		if (ly > 144)
		{
			if (Mode1Interrupt() && prevMode != Mode1_VBlank)
				memory.memory[0xff0f] |= 0x2;

			SetMode(Mode1_VBlank);
		}
		else
		{
			if (cycAcc < 81) // Mode 2 OAM
			{
				if (Mode2Interrupt() && prevMode != Mode2_OAM)
					memory.memory[0xff0f] |= 0x2; // stat interrupt

				SetMode(Mode2_OAM);
			}
			else if (cycAcc < 253) // Mode 3 Drawing
			{
				SetMode(Mode3_Drawing);
			}
			else if (cycAcc < 457) // Mode0 H - Blank
			{
				if (Mode0Interrupt() && prevMode != Mode0_HBlank)
					memory.memory[0xff0f] |= 0x2;

				SetMode(Mode0_HBlank);
			}
		}

		if (cycAcc > 252 && ly <= 144 && !lineDone)
		{
			SetBG(ly);
			SetWindow(ly);
			SetSprite(ly);
			lineDone = true;
		}

		if (ly == 144 && !frameDone)
		{
			frameDone = true;
			if (LCDEnable())
			{
				memory.memory[0xff0f] |= 0x1;
				SetTextureData();
			}

			memset(bg.cpuBuffer.data(), 0, bg.cpuBuffer.size() * 4);
			memset(window.cpuBuffer.data(), 0, window.cpuBuffer.size() * 4);
			memset(sprite.cpuBuffer.data(), 0, sprite.cpuBuffer.size() * 4);
		}

		if (cycAcc > 456)
		{
			cycAcc -= 456;
			lineDone = false;
			ly = ++memory.memory[0xff44];
		}

		if (ly == lyc)
		{
			if (LYCInterrupt())
			{
				if (!(memory.memory[0xff41] & COINCIDENCE_BIT))
				{
					SetCoincidence(true);
					memory.memory[0xff0f] |= 0x2;
				}
			}
		}
		else
			SetCoincidence(false);

		if (ly > 154)
		{
			memory.memory[0xff44] = 0;
			frameDone = false;
		}

	}

	void PPU::SetTextureData()
	{
		for (uint32_t y = 0; y < 144; y++)
		{
			for (uint32_t x = 0; x < 160; x++)
			{
				uint32_t dc = x + y * 160;
				uint32_t offset = x + y * 256;
				uint32_t value = 0x0;

				if (sprite.cpuBuffer[offset])
					value = sprite.cpuBuffer[offset];
				else if (window.cpuBuffer[offset])
					value = window.cpuBuffer[offset];
				else
					value = bg.cpuBuffer[offset];

				display.cpuBuffer[dc] = value;
			}
		}

		display.LoadAndUpdate();

		if (debugEnabled)
			SetDebugTextures();
	}

	void PPU::SetDebugTextures()
	{
		bg.LoadAndUpdate();
		window.LoadAndUpdate();
		sprite.LoadAndUpdate();

		// TODO implement
	}

	uint32_t PPU::GetPixelFromTile(uint32_t tileDataStart, uint32_t tileNumber, uint32_t x, uint32_t y, bool signedSeek) const
	{
		//  every tile is represented by 16 bytes;
		//  every 2 consecutive bytes represents a row (8 pixels)
		//  the palette to use (0 - 3) is given by taking the corresponding bits from the 2 bytes
		//  eg. for 2nd pixel from the left take bit 6 from both bytes and flip them
		//  0x8800 represents signed indexing

		uint32_t location =
			signedSeek ?
			tileDataStart + ((int8_t)tileNumber * 0x10) + ((y % 8) * 2) : // signed indexing
			tileDataStart + (tileNumber * 0x10) + ((y % 8) * 2);

		uint16_t tileData = memory.As<uint16_t>(location); // tile row

		uint32_t index = 0;
		index = ((tileData << (x % 8)) & (0x8000)) ? 0x1 : 0x0;
		index |= ((tileData << (x % 8)) & (0x0080)) ? 0x2 : 0x0;

		// index from color palette
		index = (memory[0xff47] >> (index * 2)) & 3;

		return index;
	}

	uint32_t PPU::GetSpritePixelFromTile(uint32_t tileNumber, uint32_t x, uint32_t y, uint32_t spritePalette) const
	{
		uint32_t spriteData = 0x8000;
		uint32_t location = spriteData + (tileNumber * 0x10) + ((y % 8) * 2);

		uint16_t tileData = memory.As<uint16_t>(location);

		uint32_t index = 0;
		index = ((tileData << (x % 8)) & (0x8000)) ? 0x1 : 0x0;
		index |= ((tileData << (x % 8)) & (0x0080)) ? 0x2 : 0x0;

		// index from color palette
		index = (spritePalette >> (index * 2)) & 3;
		return index;
	}

	uint32_t PPU::GetTileMapOffset(uint32_t x, uint32_t y) const
	{
		return ((y / 8) * 4) + (x / 8);
	}

	void PPU::SetBG(uint32_t row)
	{
		uint32_t bgTileData = TileData();
		uint32_t bgTileMap = BGTileMap();

		uint32_t scx = memory[0xff43];
		uint32_t scy = memory[0xff42];

		uint32_t y = (row + scy) % 256;

		bool signedSeek = bgTileData == 0x8800;

		for (uint32_t i = 0; i < 256; i++)
		{
			uint32_t x = (i + scx) % 256;

			// 32 x (32 byte) tilemap -> index of tiles
			uint32_t tileNumber = memory[bgTileMap + GetTileMapOffset(x, y)];

			uint32_t index = GetPixelFromTile(bgTileData, tileNumber, x, y, signedSeek);

			bg.cpuBuffer[row * 256 + i] = color_palette[index];
		}
	}

	void PPU::SetWindow(uint32_t row)
	{
		if (!WindowEnable())
			return;

		uint32_t windowTileData = TileData();
		uint32_t windowTileMap = WindowTileMap();

		uint32_t wx = memory[0xff4b] - 7;
		uint32_t wy = memory[0xff4a];

		uint32_t y = row - wy;

		if (y > 144)
			return;

		for (uint32_t i = wx; i < glm::min(256u, wx + 160); i++)
		{
			uint32_t x = i - wx;

			uint32_t tileNumber = memory[windowTileMap + GetTileMapOffset(x, y)];

			uint32_t index = GetPixelFromTile(windowTileData, tileNumber, x, y, windowTileData == 0x8800);

			window.cpuBuffer[row * 256 + i] = color_palette[index];
		}
	}

	void PPU::SetSprite(uint32_t row)
	{
		if (!SpriteEnable())
			return;

		uint32_t oam = 0xfe00;

		uint32_t height = SpriteYSize();

		for (uint32_t k = oam; k < 0xfea0; k += 4)
		{
			uint32_t y = memory[k] - 16;
			uint32_t x = memory[k + 1] - 8;
			uint32_t tileNumber = memory[k + 2];
			uint32_t flags = memory[k + 3];

			bool xflip = flags & (1 << 5);
			bool yflip = flags & (1 << 6);

			uint32_t palette = flags & (1 << 4) ? memory[0xff49] : memory[0xff48];

			if (row < y || row > (y + height))
				continue;

			if (yflip)
			{
				uint32_t ymax = y + height - 1;
				y += (ymax - row);
			}
			else
				y = row;

			for (uint32_t i = 0; i < 8; i++)
			{
				uint32_t index = GetSpritePixelFromTile(tileNumber, xflip ? x + (7 - i) : x + i, y, palette);

				sprite.cpuBuffer[row * 256 + x + i] = color_palette[index];
			}
		}
	}
}