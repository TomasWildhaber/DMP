#include "pch.h"
#include "AsioSession.h"
#include "AsioUtilities.h"
#include "Core/Application.h"
#include "Event/NetworkEvent.h"

namespace Core
{
	Ref<Session> Session::Create(Context* context, Socket* socket, MessageQueue& inputMessageQueue)
	{
		return new AsioSession(((AsioContext*)context)->context, std::move(((AsioSocket*)socket)->socket), inputMessageQueue);
	}

	AsioSession::AsioSession(asio::io_context& Context, asio::ip::tcp::socket Socket, MessageQueue& inputMessageQueue) : Session(), context(Context), socket(std::move(Socket)), inputMessageQueue(inputMessageQueue)
	{
		ReadMessagePackets();
	}

	void Core::AsioSession::SendMessagePackets(Ref<Message>& message)
	{
		asio::post(context, [this, message]()
		{
			if (!outputMessageQueue.GetCount())
			{
				outputMessageQueue.Add(message);
				SendMessageQueue();
			}
			else
				outputMessageQueue.Add(message);
		});
	}

	void AsioSession::SendMessageQueue()
	{
		asio::async_write(socket, asio::buffer(&outputMessageQueue.Get().Header, sizeof(MessageHeader)), [&](std::error_code errorCode, std::size_t length)
		{
			if (errorCode)
			{
				Disconnect();
				return;
			}

			asio::async_write(socket, asio::buffer(outputMessageQueue.Get().Body.Content->GetDataAs<uint8_t>(), outputMessageQueue.Get().Header.Size), [&](asio::error_code errorCode, std::size_t length)
			{
				if (errorCode)
				{
					Disconnect();
					return;
				}

				MessageSentEvent event(outputMessageQueue.Get());
				Application::Get().OnEvent(event);

				outputMessageQueue.Pop();

				if (outputMessageQueue.GetCount())
					SendMessageQueue();
			});
		});
	}

	void AsioSession::ReadMessagePackets()
	{
		asio::async_read(socket, asio::buffer(&tempMessage->Header, sizeof(MessageHeader)), [&](std::error_code errorCode, std::size_t length)
		{
			if (errorCode)
			{
				Disconnect();
				return;
			}

			tempMessage->Body.Content = CreateRef<Buffer>(tempMessage->Header.Size);
			tempMessage->Header.SessionId = id;

			asio::async_read(socket, asio::buffer(tempMessage->Body.Content->GetDataAs<uint8_t>(), tempMessage->Header.Size), [&](std::error_code errorCode, std::size_t length)
			{
				if (errorCode)
				{
					Disconnect();
					return;
				}

				inputMessageQueue.Add(tempMessage);

				MessageAcceptedEvent event(tempMessage.Get());
				Application::Get().OnEvent(event);

				tempMessage = CreateRef<Message>();

				ReadMessagePackets();
			});
		});
	}

	void AsioSession::Disconnect()
	{
		asio::post(context, [this]() { socket.close(); });

		DisconnectedEvent event;
		Application::Get().OnEvent(event);
	}
}