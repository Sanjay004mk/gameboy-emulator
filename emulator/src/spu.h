#pragma once

#include "rom.h"
#include <miniaudio.h>

#include <readerwriterqueue.h>

namespace emu
{
	struct SC
	{
		Memory& memory;
		uint32_t nrOffset = 0;
		uint32_t frameIndex = 0;
		uint32_t volume = 0;
		uint32_t frequency = 0;
		uint32_t shadowFrequency = 0;
		uint32_t dutyIndex = 0;

		SC(Memory& memory, uint32_t nrOffset);
		virtual ~SC() = default;

		virtual void step(uint32_t cycles) = 0;

		virtual uint32_t GetFrequency();
		virtual void SetFrequency(uint32_t frequency);

		virtual uint32_t GetVoulme();
		virtual bool IsVolumeAdd();
		virtual uint32_t GetVolumeSweepPeriod();

		virtual uint32_t GetLengthCounter();

		virtual uint32_t GetDutyIndex();
		virtual uint32_t GetCurrentDuty();

		uint8_t& nrx0();
		uint8_t& nrx1();
		uint8_t& nrx2();
		uint8_t& nrx3();
		uint8_t& nrx4();
	};

	struct SC1 : public SC
	{
		SC1(Memory& memory);
		void step(uint32_t cycles) override;
	};

	struct SC2 : public SC
	{
		SC2(Memory& memory);
		void step(uint32_t cycles) override;
	};

	struct SC3 : public SC
	{
		SC3(Memory& memory);
		void step(uint32_t cycles) override;
	};

	struct SC4 : public SC
	{
		SC4(Memory& memory);
		void step(uint32_t cycles) override;
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

		void WriteCallback(uint32_t address, uint8_t value);

	private:
		Memory& memory;
		SC* sc[4] = {};

		uint32_t cycAcc = 0;

		moodycamel::ReaderWriterQueue<float> queue;

		ma_device audioDevice = {};
	};
}