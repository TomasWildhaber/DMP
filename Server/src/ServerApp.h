#pragma once
#include "Core/Application.h"
#include "Networking/NetworkServerInterface.h"
#include "Networking/MessageQueue.h"
#include "Networking/Session.h"
#include "Database/DatabaseInterface.h"

namespace Server
{
	class ServerApp : public Core::Application
	{
	public:
		ServerApp(const Core::ApplicationSpecifications& specs);

		virtual void OnEvent(Core::Event& e) override;
	private:
		void OnClientConnected(Core::ConnectedEvent& e);
		void OnClientDisconnected(Core::DisconnectedEvent& e);
		void OnMessageSent(Core::MessageSentEvent& e);
		void OnMessageAccepted(Core::MessageAcceptedEvent& e);

		void ProcessMessageQueue();
		void ProcessMessage();

		void SendResponse(Core::Response& response);
		void SendResponseToAllClients(Core::Response& response);
		void SendUpdateResponse(std::string& tableName);

		Ref<Core::NetworkServerInterface> networkInterface;
		std::deque<Ref<Core::Session>> sessions;
		Core::MessageQueue messageQueue;

		Ref<Core::DatabaseInterface> databaseInterface;
	};
}