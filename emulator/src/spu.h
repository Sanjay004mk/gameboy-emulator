#pragma once

#include "rom.h"
#include <miniaudio.h>

#include <readerwriterqueue.h>

namespace emu
{
	struct SC
	{
		Memory& memory;
		bool enabled = false;
		uint32_t nrOffset = 0;
		uint32_t cycles = 0;
		uint32_t frameIndex = 0;
		uint32_t volume = 0;
		int32_t dutyTimer = 0;
		uint32_t dutyIndex = 0;
		int32_t lengthCounter = 0;
		int32_t volumeEnvelope = 0, volumeEnvelopePeriod = 0; bool updateVolume = false;

		SC(Memory& memory, uint32_t nrOffset);
		virtual ~SC() = default;

		void step(uint32_t cycles);
		virtual void TimerTick();
		virtual void FrameStep() = 0;
		virtual float Sample() = 0;

		virtual void LengthCounterStep();
		virtual void VolumeEnvelopeStep();

		virtual uint32_t GetFrequency();
		virtual void SetFrequency(uint32_t frequency);

		virtual uint32_t GetVolume();
		virtual bool IsVolumeAdd();
		virtual uint32_t GetVolumeSweepPeriod();

		virtual uint32_t GetLengthCounter();
		virtual bool GetLengthEnable();

		virtual uint32_t GetDutyIndex();
		virtual uint32_t GetCurrentDuty();

		virtual void Trigger();

		uint8_t& nrx0();
		uint8_t& nrx1();
		uint8_t& nrx2();
		uint8_t& nrx3();
		uint8_t& nrx4();
		uint8_t& nrx(uint32_t i);
	};

	struct SC1 : public SC
	{
		uint32_t shadowFrequency = 0;
		int32_t sweepPeriod = 0;

		SC1(Memory& memory);
		void FrameStep() override;
		float Sample() override;
		void FrequencySweepStep();
	};

	struct SC2 : public SC
	{
		SC2(Memory& memory);
		void FrameStep() override;
		float Sample() override;
	};

	struct SC3 : public SC
	{
		SC3(Memory& memory);
		void FrameStep() override;
		float Sample() override;
		uint32_t GetVolume() override;
		void TimerTick() override;
	};

	struct SC4 : public SC
	{
		uint16_t lfsr = 0x0;

		SC4(Memory& memory);
		void FrameStep() override;
		float Sample() override;
		void TimerTick() override;
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