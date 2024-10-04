#pragma once
#include "rdrapi.h"

#include "renderer/keycodes.h"

namespace rdr
{
	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, // WindowFocus, WindowLostFocus, WindowMoved,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
		// TODO add controller
	};

	class RDRAPI Event
	{
	public:
		bool handled = false;

		virtual ~Event() = default;

		virtual EventType GetEventType() const = 0;
		virtual std::string_view GetName() const = 0;
		virtual std::string ToString() const { return std::string(GetName()); }
	};

	using EventCallbackFunction = std::function<void(Event& e)>;

	class RDRAPI EventDispatcher
	{
	public:
		EventDispatcher(Event& e)
			: mEvent(e)
		{ }

		template <typename T, typename F>
		void Dispatch(const F& func)
		{
			if (mEvent.GetEventType() == T::GetEventTypeStatic())
				mEvent.handled |= func(static_cast<T&>(mEvent));
		}

	private:
		Event& mEvent;
	};

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
virtual EventType GetEventType() const override { return EventType::type; }\
virtual std::string_view GetName() const override { return #type; }

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(uint32_t width, uint32_t height)
			: mWidth(width), mHeight(height)
		{}

		uint32_t GetWidth() const { return mWidth; }
		uint32_t GetHeight() const { return mHeight; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << mWidth << ", " << mHeight;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)

	private:
		uint32_t mWidth, mHeight;
	};

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		EVENT_CLASS_TYPE(WindowClose)
	};

	class KeyEvent : public Event
	{
	public:
		KeyCode GetKeyCode() const { return mKeyCode; }

	protected:
		KeyEvent(KeyCode code)
			: mKeyCode(code)
		{}

		KeyCode mKeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(KeyCode code, bool isRepeated = false)
			: KeyEvent(code), mIsRepeated(isRepeated)
		{}

		bool IsRepeated() const { return mIsRepeated; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << mKeyCode << " [REPEATED = " << mIsRepeated << "]";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)

	private:
		bool mIsRepeated;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(KeyCode code)
			: KeyEvent(code)
		{}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << mKeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(KeyCode code)
			: KeyEvent(code)
		{}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyTypedEvent: " << (char)mKeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};

	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(float posX, float posY)
			: mX(posX), mY(posY)
		{}

		float GetX() const { return mX; }
		float GetY() const { return mY; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << mX << ", " << mY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)

	private:
		float mX, mY;
	};

	/* input mouse scrolled event */
	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float xoffs, float yoffs)
			: mXOffs(xoffs), mYOffs(yoffs)
		{}

		float GetXOffset() const { return mXOffs; }
		float GetYOffset() const { return mYOffs; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << mXOffs << ", " << mYOffs;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)

	private:
		float mXOffs, mYOffs;
	};

	/* input mouse button base event */
	class MouseButtonEvent : public Event
	{
	public:
		MouseCode GetMouseButton() const { return mMouseCode; }

	protected:
		MouseButtonEvent(MouseCode code)
			: mMouseCode(code)
		{}

		MouseCode mMouseCode;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(MouseCode code)
			: MouseButtonEvent(code)
		{}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << mMouseCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(MouseCode code)
			: MouseButtonEvent(code)
		{}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << mMouseCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}

template <typename ostream>
ostream& operator<<(ostream& stream, const rdr::Event& e)
{
	return stream << e.ToString();
}

#undef EVENT_CLASS_TYPE