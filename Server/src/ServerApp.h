#pragma once
#include "Core/Application.h"
#include "Networking/NetworkServerInterface.h"
#include "Networking/MessageQueue.h"
#include "Networking/Session.h"
#include "Database/DatabaseInterface.h"
#include "Utils/File.h"

namespace Server
{
	class ServerApp : public Core::Application
	{
	public:
		ServerApp(const Core::ApplicationSpecifications& specs);

		virtual void OnEvent(Core::Event& e) override;
	private:
		// Config file methods
		void ReadConfigFile() override;
		void WriteConfigFile() override;

		// Event functions
		void OnClientConnected(Core::ConnectedEvent& e);
		void OnClientDisconnected(Core::DisconnectedEvent& e);
		void OnMessageSent(Core::MessageSentEvent& e);
		void OnMessageAccepted(Core::MessageAcceptedEvent& e);

		// Networking methods
		void ProcessMessageQueue() override;
		void ProcessMessage();

		void SendResponse(Core::Response& response);
		void SendResponseToAllClients(Core::Response& response);
		void SendUpdateResponse(std::string& tableName);
		void SendFile(File& file);

		std::filesystem::path dir = std::filesystem::current_path() / "Attachments";

		Ref<Core::NetworkServerInterface> networkInterface;
		std::deque<Ref<Core::Session>> sessions;
		Core::MessageQueue messageQueue;

		Ref<Core::DatabaseInterface> databaseInterface;

		uint32_t port = 0;
	};
}