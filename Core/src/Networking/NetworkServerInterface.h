#pragma once
#include "Utils/Memory.h"
#include "Utils/Buffer.h"
#include "MessageQueue.h"
#include "Session.h"

namespace Core
{
	class NetworkServerInterface
	{
	public:
		virtual ~NetworkServerInterface() = default;

		inline virtual const std::error_code& GetErrorCode() const = 0;

		virtual void SendMessagePackets(Ref<Message>& message) = 0;

		virtual Ref<Session> FindSessionById(uint32_t SessionId) = 0;

		static Ref<NetworkServerInterface> Create(uint16_t port, Core::MessageQueue& inputMessageQueue, std::deque<Ref<Core::Session>>& sesionQueue);
	};
}