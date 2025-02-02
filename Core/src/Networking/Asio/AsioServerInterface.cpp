#include "pch.h"
#include "AsioServerInterface.h"
#include "AsioUtilities.h"
#include "Core/Application.h"

namespace Core
{
	Ref<NetworkServerInterface> NetworkServerInterface::Create(uint16_t port, Core::MessageQueue& inputMessageQueue, std::deque<Ref<Core::Session>>& sesionQueue)
	{
		return new AsioServerInterface(port, inputMessageQueue, sesionQueue);
	}

	AsioServerInterface::AsioServerInterface(uint16_t port, Core::MessageQueue& inputMessageQueue, std::deque<Ref<Core::Session>>& sesionQueue) : inputMessageQueue(inputMessageQueue), sessions(sesionQueue), acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
	{
		AcceptClient();
		contextThread = std::thread([&]() { context.run(); });
	}

	AsioServerInterface::~AsioServerInterface()
	{
		context.stop();

		if (contextThread.joinable())
			contextThread.join();
	}

	void AsioServerInterface::SendMessagePackets(Ref<Message>& message)
	{
		Ref<Session> session = FindSessionById(message->GetSessionId());

		if (session && session->IsOpen())
			session->SendMessagePackets(message);
		else
			sessions.erase(std::remove(sessions.begin(), sessions.end(), session), sessions.end());
	}

	void AsioServerInterface::SendMessagePacketsToAllClients(Ref<Message>& message)
	{
		for (Ref<Session> session : sessions)
		{
			if (session && session->IsOpen())
				session->SendMessagePackets(message);
		}
	}

	Ref<Session> AsioServerInterface::FindSessionById(uint32_t SessionId)
	{
		auto session = std::find_if(sessions.begin(), sessions.end(), [&](Ref<Session> session) {
			return session->GetId() == SessionId;
		});

		return session != sessions.end() ? *session : Ref<Session>();
	}

	void AsioServerInterface::AcceptClient()
	{
		acceptor.async_accept([this](std::error_code errorCode, asio::ip::tcp::socket socket)
		{
			if (!errorCode)
			{
				const std::string& sessionDomain = socket.remote_endpoint().address().to_string();
				uint16_t port = socket.remote_endpoint().port();

				AsioContext con(context);
				AsioSocket soc(socket);

				Ref<Session> session = Session::Create(&con, &soc, inputMessageQueue);
				sessions.push_back(session);

				ConnectedEvent event(sessionDomain.c_str(), port);
				Application::Get().OnEvent(event);
			}

			AcceptClient();
		});
	}
}