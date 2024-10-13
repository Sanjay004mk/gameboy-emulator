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
		bool enableCopy = true;
		BufferType type = BufferType::StagingBuffer;
		BufferImplementationInformation* impl = nullptr;
	};

	class RDRAPI Buffer
	{
	public:
		Buffer(const BufferConfiguration& config, const void* data = nullptr);
		~Buffer();

		void* GetData() const;

		template <typename T>
		T* GetDataAs() const { return static_cast<T*>(GetData()); }

		void Map();
		void UnMap();
		void Flush();

		static void SetDataUsingStagingBuffer(
			Buffer* to, 
			const void* data, 
			uint32_t size = 0, uint32_t offset = 0, 
			bool async = true
		);
		static void Copy(
			Buffer* to, Buffer* from, 
			uint32_t size = std::numeric_limits<uint32_t>::max(), 
			uint32_t srcOffset = 0, uint32_t dstOffset = 0, 
			bool async = true
		);

		const BufferConfiguration& GetConfig() const { return mConfig; }

	private:
		BufferConfiguration mConfig;

	};
}