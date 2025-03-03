#pragma once
#include <asio.hpp>
#include "Networking/NetworkClientInterface.h"

namespace Core
{
	class AsioClientInterface : public NetworkClientInterface
	{
	public:
		AsioClientInterface(const char* domain, uint16_t port, MessageQueue& inputMessageQueue);
		~AsioClientInterface();

		inline virtual const std::error_code& GetErrorCode() const override { return errorCode; };
		inline virtual const bool IsConnected() const override { return socket->is_open() && isConnected; };

		virtual void Reconnect() override;
		virtual void Disconnect() override;

		virtual void SendMessagePackets(Ref<Message>& message) override;
		virtual void ReadMessagePackets() override;
	private:
		void Connect(const asio::ip::tcp::resolver::results_type& endpoints, uint16_t port);
		void SendMessageQueue();

		asio::error_code errorCode;
		asio::io_context context;

		asio::ip::tcp::endpoint endpoint;
		Ref<asio::ip::tcp::socket> socket;

		std::thread contextThread;

		Ref<Message> tempMessage = CreateRef<Message>();
		MessageQueue& inputMessageQueue;
		MessageQueue outputMessageQueue;

		const char* domain;
		uint16_t port;

		bool isConnected = false;
	};
}