#pragma once
#include <asio.hpp>
#include "Networking/Session.h"

namespace Core
{
	class AsioSession : public Session
	{
	public:
		AsioSession(asio::io_context& Context, asio::ip::tcp::socket Socket, MessageQueue& inputMessageQueue);

		inline virtual const bool IsOpen() const override { return socket.is_open(); };

		virtual void SendMessagePackets(Ref<Message>& message) override;
		virtual void ReadMessagePackets() override;

		virtual void Disconnect() override;
	private:
		void SendMessageQueue();

		asio::ip::tcp::socket socket;
		asio::io_context& context;

		Ref<Message> tempMessage = CreateRef<Message>();
		Core::MessageQueue& inputMessageQueue;
		Core::MessageQueue outputMessageQueue;
	};
}