#pragma once
#include "Event.h"
#include "Networking/Message.h"

namespace Core
{
	class NetworkEvent : public Event
	{
	protected:
		NetworkEvent() {}
	};

	class MessageSentEvent : public NetworkEvent
	{
	public:
		MessageSentEvent(Message& _message) : message(_message) {}

		inline const Message& GetMessage() const { return message; }

		static EventType GetStaticEventType() { return EventType::MessageSent; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "MessageSent"; }
	private:
		Message& message;
	};

	class MessageAcceptedEvent : public NetworkEvent
	{
	public:
		MessageAcceptedEvent(Message& _message) : message(_message) {}

		inline Message& GetMessage() const { return message; }

		static EventType GetStaticEventType() { return EventType::MessageAccepted; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "MessageAccepted"; }
	private:
		Message& message;
	};

	class ConnectedEvent : public NetworkEvent
	{
	public:
		ConnectedEvent(const char* _domain, uint16_t _port) : domain(_domain), port(_port) {}

		inline const char* GetDomain() const { return domain; }
		inline const uint16_t GetPort() const { return port; }

		static EventType GetStaticEventType() { return EventType::ConnectedEvent; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "Connected"; }
	private:
		const char* domain;
		uint16_t port;
	};

	class DisconnectedEvent : public NetworkEvent
	{
	public:
		DisconnectedEvent() {}

		static EventType GetStaticEventType() { return EventType::DisconnectedEvent; }
		EventType GetEventType() const override { return GetStaticEventType(); }
		inline const char* GetName() const override { return "Disconnected"; }
	};
}