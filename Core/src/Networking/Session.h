#pragma once
#include "Utilities.h"
#include "MessageQueue.h"

namespace Core
{
	class Session
	{
	public:
		Session() : id(idCounter++) {}

		inline const uint32_t GetId() const { return id; }
		inline virtual const bool IsOpen() const = 0;

		virtual void SendMessagePackets(Ref<Message>& message) = 0;
		virtual void ReadMessagePackets() = 0;

		virtual void Disconnect() = 0;

		static Ref<Session> Create(Context* context, Socket* socket, MessageQueue& inputMessageQueue);
	protected:
		uint32_t id = 0;

		inline static uint32_t idCounter = 1;
	};
}