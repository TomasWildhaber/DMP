#pragma once
#include "Utils/Memory.h"
#include "Utils/Buffer.h"
#include "Networking/MessageQueue.h"

namespace Core
{
	class NetworkClientInterface
	{
	public:
		virtual ~NetworkClientInterface() = default;

		inline virtual const std::error_code& GetErrorCode() const = 0;
		inline virtual const bool IsConnected() const = 0;

		virtual void Reconnect() = 0;
		virtual void Disconnect() = 0;

		virtual void SendMessagePackets(Ref<Message>& message) = 0;
		virtual void ReadMessagePackets() = 0;

		static Ref<NetworkClientInterface> Create(const char* domain, uint16_t port, MessageQueue& inputMessageQueue);
		static Ref<NetworkClientInterface> Create(std::string& domain, uint16_t port, MessageQueue& inputMessageQueue);
	};
}