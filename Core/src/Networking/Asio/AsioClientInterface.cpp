#include "pch.h"
#include "AsioClientInterface.h"
#include <asio/executor_work_guard.hpp>
#include "Debugging/Log.h"
#include "Event/NetworkEvent.h"
#include "Core/Application.h"

namespace Core
{
	Ref<NetworkClientInterface> NetworkClientInterface::Create(const char* domain, uint16_t port, MessageQueue& inputMessageQueue)
	{
		return new AsioClientInterface(domain, port, inputMessageQueue);
	}
	
	AsioClientInterface::AsioClientInterface(const char* domain, uint16_t port, MessageQueue& inputMessageQueue) : inputMessageQueue(inputMessageQueue)
	{
		asio::ip::tcp::resolver resolver(context);
		asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(domain, std::to_string(port));

		socket = new asio::ip::tcp::socket(context);
		
		asio::async_connect(socket.Get(), endpoints, [&](std::error_code errorCode, asio::ip::tcp::endpoint endpoint)
		{
			if (!errorCode)
			{
				const std::string& _domain = socket->remote_endpoint().address().to_string();
				ConnectedEvent event(_domain.c_str(), port);
				Application::Get().OnEvent(event);

				ReadMessagePackets();
			}
			else
				ERROR("Failed to connect: {0}", errorCode.message());
		});

		contextThread = std::thread([&]() { context.run(); });
	}

	AsioClientInterface::~AsioClientInterface()
	{
		context.stop();

		if (contextThread.joinable())
			contextThread.join();
	}

	void AsioClientInterface::SendMessagePackets(Ref<Message>& message)
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

	void AsioClientInterface::SendMessageQueue()
	{
		asio::async_write(socket.Get(), asio::buffer(&outputMessageQueue.Get().Header, sizeof(MessageHeader)), [&](std::error_code errorCode, std::size_t length)
		{
			if (errorCode)
			{
				Disconnect();
				return;
			}

			asio::async_write(socket.Get(), asio::buffer(outputMessageQueue.Get().Body.Content->GetDataAs<uint8_t>(), outputMessageQueue.Get().Header.Size), [&](std::error_code errorCode, std::size_t length)
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

	void AsioClientInterface::ReadMessagePackets()
	{
		asio::async_read(socket.Get(), asio::buffer(&tempMessage->Header, sizeof(tempMessage->Header)), [&](std::error_code errorCode, std::size_t length)
		{
			if (errorCode)
			{
				Disconnect();
				return;
			}

			tempMessage->Body.Content = CreateRef<Buffer>(tempMessage->Header.Size);

			asio::async_read(socket.Get(), asio::buffer(tempMessage->Body.Content->GetDataAs<uint8_t>(), tempMessage->Header.Size), [&](std::error_code errorCode, std::size_t length)
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

	void AsioClientInterface::Disconnect()
	{
		asio::post(context, [this]() { socket->close(); });

		DisconnectedEvent event;
		Application::Get().OnEvent(event);
	}
}