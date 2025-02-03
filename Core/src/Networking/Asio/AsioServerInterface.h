#pragma once
#include <asio.hpp>
#include "Networking/NetworkServerInterface.h"
#include "Networking/Session.h"

namespace Core
{
	class AsioServerInterface : public Core::NetworkServerInterface
	{
	public:
		AsioServerInterface(uint16_t port, Core::MessageQueue& inputMessageQueue, std::deque<Ref<Core::Session>>& sesionQueue);
		~AsioServerInterface();

		inline virtual const std::error_code& GetErrorCode() const override { return errorCode; }

		virtual void DisconnectAllClients() override;

		virtual void SendMessagePackets(Ref<Message>& message) override;
		virtual void SendMessagePacketsToAllClients(Ref<Message>& message) override;

		virtual Ref<Session> FindSessionById(uint32_t SessionId) override;
	private:
		void AcceptClient();

		asio::error_code errorCode;
		asio::io_context context;
		asio::ip::tcp::acceptor acceptor;

		std::thread contextThread;

		Core::MessageQueue& inputMessageQueue;
		std::deque<Ref<Session>>& sessions;
	};
}