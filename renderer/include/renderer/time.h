#pragma once
#include "renderer/rdrapi.h"

namespace rdr
{
	class Time
	{
	public:
		static RDRAPI float GetTime();
		static RDRAPI void SetTime(float time);

	private:
	};
}