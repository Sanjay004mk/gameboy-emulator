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
		glm::u32vec2 size;
		TextureFormat format;
		TextureType type;
		SamplerType sampler;
		TextureImplementationInformation* impl = nullptr;
	};

	class Texture
	{
	public:
		Texture(const TextureConfiguration& config);
		~Texture();

		const TextureConfiguration& GetConfig() const { return mConfig; }

	private:
		TextureConfiguration mConfig;
	};
}