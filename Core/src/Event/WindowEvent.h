#pragma once
#include "Event.h"

namespace Core
{
	class WindowEvent : public Event
	{
	protected:
		WindowEvent() {}
	};

	class WindowClosedEvent : public WindowEvent
	{
	public:
		WindowClosedEvent() {}

		static EventType GetStaticEventType() { return EventType::WindowClosed; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "WindowClosed"; }
	};

	class WindowFocusedEvent : public WindowEvent
	{
	public:
		WindowFocusedEvent(int& _focused) : focused(_focused) {}

		inline const int GetFocus() const { return focused; }

		static EventType GetStaticEventType() { return EventType::WindowFocused; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "WindowFocus"; }
	private:
		int focused;
	};

	class WindowResizedEvent : public WindowEvent
	{
	public:
		WindowResizedEvent(int& Width, int& Height) : width(Width), height(Height) {}

		inline const int GetWidth() const { return width; }
		inline const int GetHeight() const { return height; }

		static EventType GetStaticEventType() { return EventType::WindowResized; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "WidowResized"; }
	private:
		int width, height;
	};

	class WindowMovedEvent : public WindowEvent
	{
	public:
		WindowMovedEvent(int& _x, int& _y) : x(_x), y(_y) {}

		inline const int GetX() const { return x; }
		inline const int GetY() const { return y; }

		static EventType GetStaticEventType() { return EventType::WindowMoved; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "WidowMoved"; }
	private:
		int x, y;
	};
}