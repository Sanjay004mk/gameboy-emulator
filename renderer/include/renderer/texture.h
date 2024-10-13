#pragma once

#include <glm/glm.hpp>

#include "renderer/rdrapi.h"
#include "renderer/renderinfo.h"
#include "renderer/buffer.h"

namespace rdr
{
	struct TextureImplementationInformation;

	struct TextureConfiguration
	{
		glm::u32vec2 size = {};
		TextureFormat format;
		TextureType type;
		SamplerType sampler;
		TextureImplementationInformation* impl = nullptr;
	};

	struct TextureBlitInformation
	{
		glm::ivec2 srcMin{}, srcMax{};
		glm::ivec2 dstMin{}, dstMax{};
		SamplerFilter filter = SamplerFilter::Linear;
	};

	class RDRAPI Texture
	{
	public:
		Texture(const char* file);
		Texture(const char* file, const TextureConfiguration& config);
		Texture(const TextureConfiguration& config);
		~Texture();

		const TextureConfiguration& GetConfig() const { return mConfig; }

		size_t GetByteSize() const;

		void SetData(const void* data, bool async = true);
		void SetData(const Buffer* buffer, bool async = true);
		void LoadDataToBuffer(Buffer* buffer, bool async = true);

	private:
		void Init();
		void LoadFromFile(const char* file);

		TextureConfiguration mConfig;
	};
}