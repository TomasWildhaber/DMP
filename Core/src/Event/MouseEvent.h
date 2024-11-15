#pragma once
#include "Event.h"

namespace Core
{
	class MouseEvent : public Event
	{
	protected:
		MouseEvent() {}
	};

	class MouseButtonPressedEvent : public MouseEvent
	{
	public:
		MouseButtonPressedEvent(int& _button) : button(_button) {}
		inline const int GetMouseButton() const { return button; }

		static EventType GetStaticEventType() { return EventType::MouseButtonPressed; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "MouseButtonPressed"; }
	private:
		int button;
	};

	class MouseButtonReleasedEvent : public MouseEvent
	{
	public:
		MouseButtonReleasedEvent(int& _button) : button(_button) {}
		inline const int GetMouseButton() const { return button; }

		static EventType GetStaticEventType() { return EventType::MouseButtonReleased; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "MouseButtonReleased"; }
	private:
		int button;
	};

	class MouseMovedEvent : public MouseEvent
	{
	public:
		MouseMovedEvent(float _x, float _y) : x(_x), y(_y) {}

		inline const float GetX() const { return x; }
		inline const float GetY() const { return y; }

		static EventType GetStaticEventType() { return EventType::MouseMoved; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "MouseMoved"; }
	private:
		float x, y;
	};

	class MouseScrolledEvent : public MouseEvent
	{
	public:
		MouseScrolledEvent(float _xScroll, float _yScroll) : xScroll(_xScroll), yScroll(_yScroll) {}

		inline const float GetXScroll() const { return xScroll; }
		inline const float GetYScroll() const { return yScroll; }

		static EventType GetStaticEventType() { return EventType::MouseScrolled; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "MouseScrolled"; }
	private:
		float xScroll, yScroll;
	};
}