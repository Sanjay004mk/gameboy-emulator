#pragma once

#include "rom.h"
#include <miniaudio.h>

#include <readerwriterqueue.h>

namespace emu
{
	class SC
	{
	public:
		SC(Memory& memory);
		virtual ~SC() = default;

		virtual void step(uint32_t cycles) = 0;
	protected:
		Memory& memory;
	};

	class SC1 : public SC
	{
	public:
		SC1(Memory& memory);
		void step(uint32_t cycles) override;

	private:
	};

	class SC2 : public SC
	{
	public:
		SC2(Memory& memory);
		void step(uint32_t cycles) override;

	private:
	};

	class SC3 : public SC
	{
	public:
		SC3(Memory& memory);
		void step(uint32_t cycles) override;

	private:
	};

	class SC4 : public SC
	{
	public:
		SC4(Memory& memory);
		void step(uint32_t cycles) override;

	private:
	};

	class SPU
	{
	public:
		static inline constexpr uint32_t SAMPLE_RATE = 48000;

		SPU(Memory& memory);
		~SPU();

		void step(uint32_t cycles);

		moodycamel::ReaderWriterQueue<float>& GetQueue() { return queue; }

		void Sample();

	private:
		Memory& memory;
		SC* sc[4] = {};

		float start = 0.f;

		moodycamel::ReaderWriterQueue<float> queue;

		ma_device audioDevice = {};
	};
}