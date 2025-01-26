#pragma once

#include <renderer/rdr.h>

namespace emu
{
	class PPU
	{
	public:
		enum PPUMode
		{
			Mode0_HBlank,
			Mode1_VBlank,
			Mode2_OAM,
			Mode3_Drawing
		};

		PPU(Memory& memory);
		~PPU();

		void step(uint32_t cycles);

		rdr::Texture* GetDisplayTexture() { return display.texture; }

		bool LCDEnable() const
		{
			return memory[0xff40] & LCD_ENABLE_BIT;
		}

		bool WindowEnable() const
		{
			return memory[0xff40] & WINDOW_ENALBLE_BIT;
		}

		bool SpriteEnable() const
		{
			return memory[0xff40] & SPRITE_ENALBE_BIT;
		}

		bool PriorityEnable() const
		{
			return memory[0xff40] & ENABLE_PRIORITY_BIT;
		}

		uint32_t SpriteYSize() const
		{
			return (memory[0xff40] & SPRITE_SIZE_SELECT_BIT) ? 16 : 8;
		}

		uint32_t WindowTileMap() const
		{
			return (memory[0xff40] & WINDOW_TILE_MAP_SELECT_BIT) ? 0x9c00 : 0x9800;
		}

		uint32_t TileData() const
		{
			return (memory[0xff40] & TILE_DATA_SELECT_BIT) ? 0x8000 : 0x8800;
		}

		uint32_t BGTileMap() const
		{
			return (memory[0xff40] & BG_TILE_MAP_SELECT_BIT) ? 0x9c00 : 0x9800;
		}

		bool LYCInterrupt() const
		{
			return memory[0xff41] & LYC_COINCIDENCE_INTERRUPT_BIT;
		}

		bool Mode2Interrupt() const
		{
			return memory[0xff41] & MODE2_INTERRUPT_BIT;
		}

		bool Mode1Interrupt() const
		{
			return memory[0xff41] & MODE1_INTERRUPT_BIT;
		}

		bool Mode0Interrupt() const
		{
			return memory[0xff41] & MODE0_INTERRUPT_BIT;
		}

		void SetCoincidence(bool co)
		{
			memory.memory[0xff41] &= ~COINCIDENCE_BIT;
			memory.memory[0xff41] |= (co ? COINCIDENCE_BIT : 0);
		}

		void SetMode(PPUMode mode)
		{
			memory.memory[0xff41] &= ~MODE_BITS;
			memory.memory[0xff41] |= mode;
			prevMode = mode;
		}

		void SetDebugMode(bool isDebug) { debugEnabled = isDebug; }

	private:
		void SetTextureData();
		void SetDebugTextures();

		void SetBG(uint32_t row);
		void SetWindow(uint32_t row);
		void SetSprite(uint32_t row);

		Memory& memory;

		struct TextureMap
		{
			rdr::Texture* texture = nullptr;
			rdr::Buffer* stagingBuffer = nullptr;
			std::vector<uint32_t> cpuBuffer;

			TextureMap(uint32_t w, uint32_t h);
			~TextureMap();

			void LoadStagingBuffer();
			void UpdateTexture(bool async = false);
			void LoadAndUpdate(bool async = false) { LoadStagingBuffer(); UpdateTexture(async); }
		};

		TextureMap display, bg, window, sprite, tileMap, tiles;
		bool debugEnabled = false;

		// LCD Control - 0xff40
		static inline constexpr uint32_t LCD_ENABLE_BIT = (1 << 7);
		static inline constexpr uint32_t WINDOW_TILE_MAP_SELECT_BIT = (1 << 6);
		static inline constexpr uint32_t WINDOW_ENALBLE_BIT = (1 << 5);
		static inline constexpr uint32_t TILE_DATA_SELECT_BIT = (1 << 4);
		static inline constexpr uint32_t BG_TILE_MAP_SELECT_BIT = (1 << 3);
		static inline constexpr uint32_t SPRITE_SIZE_SELECT_BIT = (1 << 2);
		static inline constexpr uint32_t SPRITE_ENALBE_BIT = (1 << 1);
		static inline constexpr uint32_t ENABLE_PRIORITY_BIT = (1);

		//LCD Status - 0xff41
		static inline constexpr uint32_t LYC_COINCIDENCE_INTERRUPT_BIT = (1 << 6);
		static inline constexpr uint32_t MODE2_INTERRUPT_BIT = (1 << 5);
		static inline constexpr uint32_t MODE1_INTERRUPT_BIT = (1 << 4);
		static inline constexpr uint32_t MODE0_INTERRUPT_BIT = (1 << 3);
		static inline constexpr uint32_t COINCIDENCE_BIT = (1 << 2);
		static inline constexpr uint32_t MODE_BITS = (1 << 1) | 1;

		PPUMode prevMode = Mode0_HBlank;
		bool lineDone = false, frameDone = false;

		uint32_t GetPixelFromTile(uint32_t tileDataStart, uint32_t tileNumber, uint32_t x, uint32_t y, bool signedSeek = false) const;
		uint32_t GetSpritePixelFromTile(uint32_t tileNumber, uint32_t x, uint32_t y, uint32_t spritePalette) const;
		uint32_t GetTileMapOffset(uint32_t x, uint32_t y) const;

		uint32_t cycAcc = 0;

		friend class Debugger;
	};
}