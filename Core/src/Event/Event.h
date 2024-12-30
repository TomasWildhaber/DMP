#pragma once

namespace Core
{
	enum class EventType
	{
		None = 0,
		WindowClosed, WindowResized, WindowFocused, WindowMoved,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
		ConnectedEvent, DisconnectedEvent, MessageSent, MessageAccepted
	};

	class Event
	{
	public:
		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;

		template<typename T, typename F>
		static void Dispatch(Event& e, F function)
		{
			if (e.GetEventType() == T::GetStaticEventType())
				function(static_cast<T&>(e));
		}
	};
}