#pragma once
#include "Event.h"

namespace Core
{
	class KeyEvent : public Event
	{
	public:
		inline int GetKeyCode() const { return keyCode; }
	protected:
		KeyEvent(int _keyCode) : keyCode(_keyCode) {}

		int keyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(int& _keyCode, int _repeatCount) : KeyEvent(_keyCode), repeatCount(_repeatCount) {}

		static EventType GetStaticEventType() { return EventType::KeyPressed; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "KeyPressed"; }
		inline int GetRepeatCount() const { return repeatCount; }
	private:
		int repeatCount;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(int& _keyCode) : KeyEvent(_keyCode) {}

		static EventType GetStaticEventType() { return EventType::KeyReleased; }
		virtual EventType GetEventType() const override { return GetStaticEventType(); }
		virtual inline const char* GetName() const override { return "KeyReleased"; }
	};

	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(uint32_t& _keyCode) : KeyEvent(_keyCode) {}

		static EventType GetStaticEventType() { return EventType::KeyTyped; }
		virtual EventType GetEventType() const override { return GetStaticEventType(); }
		virtual inline const char* GetName() const override { return "KeyTyped"; }
	};
}