#pragma once
#include "renderer/rdrapi.h"

namespace rdr
{
	enum class DataType
	{
		Unknown = 0,
		Bool, Uint8, Int8, Char, Unorm, Snorm,
		Uint16, Int16, Uint32, Int32, Float,
		Uint64, Int64, Double, Vec2,
		Vec3,
		Vec4,
		Mat3,
		Mat4,
	};

	struct TextureFormat
	{
		TextureFormat() = default;
		TextureFormat(uint8_t channels, DataType type, bool isDepth = false)
			: channels(channels), type(type), isDepth(isDepth)
		{}

		DataType type = DataType::Uint8; // data type (size) of each channel
		uint8_t channels = 4; // no. of channels (specify 2 for depth and stencil)
		bool isDepth = false; // color or depth
	};

	struct TextureType
	{
		uint8_t data = 0xfd;

		template <uint8_t field>
		void Set(bool value)
		{
			if (value)
				data |= (field);
			else
				data &= ~(field);
		}

		template <uint8_t field>
		bool Get() const
		{
			return data & (field);
		}

		static constexpr uint8_t shaderInput = 0x01; // used as an input to a shader
		static constexpr uint8_t shaderOutput = 0x02; // used as write buffer for a fragment shader output
		static constexpr uint8_t modified = 0x04; // the data will be modified externally
		static constexpr uint8_t sampled = 0x08; // the data will be sampled in a shader

	};

	enum class SamplerFilter
	{
		Nearest = 0,
		Linear,
		Cubic,
	};

	enum class SamplerAddressMode
	{
		Repeat = 0,
		MirroredRepeat,
		ClampToEdge,
		ClampToBorder,
		MirrorClampToEdge,
	};

	struct SamplerType
	{
		SamplerFilter filter;
		SamplerAddressMode addressMode;
		uint8_t sampleCount = 1;
	};

	enum class BufferType
	{
		StagingBuffer, // Used to transfer data between cpu, gpu and other buffers
		GPUBuffer, // Used to specify buffer in gpu that will recieve data from cpu using staging buffer
		UniformBuffer, 
		VertexBuffer,
		IndexBuffer,
	};
}