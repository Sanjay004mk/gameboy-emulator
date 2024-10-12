#pragma once
#include "renderer/rdrapi.h"

namespace rdr
{
	struct BufferImplementationInformation;

	struct BufferConfiguration
	{
		uint32_t size = 0;
		bool persistentMap = false;
		bool cpuVisible = true;
		BufferType type = BufferType::StagingBuffer;
		BufferImplementationInformation* impl = nullptr;
	};

	class Buffer
	{
	public:
		Buffer(const BufferConfiguration& config, const void* data = nullptr);
		~Buffer();

		void* GetData();

		void Map();
		void UnMap();
		void Flush();

		const BufferConfiguration& GetConfig() const { return mConfig; }

	private:
		BufferConfiguration mConfig;

	};
}