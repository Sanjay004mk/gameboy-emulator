#pragma once

namespace rdr
{
	class Joystick
	{
	public:
		float GetAxis(int32_t axis = 0) const;
		bool GetButton(int32_t number = 0) const;
		glm::ivec2 GetHat(int32_t index = 0) const;
		const char* GetName() const;

		bool IsEnabled() const { return enabled; }

		operator int32_t() const { return id; }

	private:
		int32_t id = 0;
		bool enabled = false;

		friend class Renderer;
	};
}