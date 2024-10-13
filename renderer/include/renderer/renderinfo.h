#pragma once
#include "renderer/rdrapi.h"

namespace rdr
{
	using DataType = uint32_t;
	namespace eDataType
	{
		enum : DataType
		{
			Unsigned	= 0x1,
			Signed		= 0x1 << 1,

			Bits8		= 0x1 << 2,
			Bits16		= 0x1 << 3,
			Bits32		= 0x1 << 4,
			Bits64		= 0x1 << 5,

			TBool		= 0x1 << 6,
			TChar		= 0x1 << 7,
			TInt		= 0x1 << 8,
			TFloat		= 0x1 << 9,
			
			Bool		= Unsigned | Bits8  | TBool,

			Uint8		= Unsigned | Bits8  | TInt,
			Uint16		= Unsigned | Bits16 | TInt,
			Uint32		= Unsigned | Bits32 | TInt,
			Uint64		= Unsigned | Bits64 | TInt,

			Sint8		= Signed | Bits8  | TInt,
			Sint16		= Signed | Bits16 | TInt,
			Sint32		= Signed | Bits32 | TInt,
			Sint64		= Signed | Bits64 | TInt,

			Ufloat8		= Unsigned | Bits8  | TFloat,
			Ufloat16	= Unsigned | Bits16 | TFloat,
			Ufloat32	= Unsigned | Bits32 | TFloat,
			Ufloat64	= Unsigned | Bits64 | TFloat,

			Sfloat8		= Signed | Bits8  | TFloat,
			Sfloat16	= Signed | Bits16 | TFloat,
			Sfloat32	= Signed | Bits32 | TFloat,
			Sfloat64	= Signed | Bits64 | TFloat,
		};
	}

	struct TextureFormat
	{
		TextureFormat() = default;
		TextureFormat(uint8_t channels, DataType type, bool isDepth = false)
			: channels(channels), type(type), isDepth(isDepth)
		{}

		// TODO add bits per channel
		DataType type = eDataType::Ufloat8; // data type (size) of each channel
		uint8_t channels = 4; // no. of channels (specify 2 for depth and stencil)
		bool reverse = false; // RGB or BGR
		bool isDepth = false; // color or depth
	};

	struct TextureType
	{
		uint8_t data = 0b00011000;

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
		static constexpr uint8_t sampled = 0x04; // the data will be sampled in a shader
		static constexpr uint8_t copySrc = 0x08; // the data will be modified externally
		static constexpr uint8_t copyDst = 0x10; // the data will be read externally
		static constexpr uint8_t copyEnable = copyDst | copySrc;

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