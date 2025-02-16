#include "pch.h"
#include "Core/EntryPoint.h"
#include "Debugging/Log.h"
#include "ServerApp.h"
#include "Database/Command.h"
#include "Database/Response.h"

namespace Server
{
	ServerApp::ServerApp(const Core::ApplicationSpecifications& specs) : Core::Application(specs)
	{
		SetLoggerTitle("Server");

		LoadConfig();

		databaseInterface = Core::DatabaseInterface::Create("tcp://127.0.0.1:3306", "dmp", "dmp", "Tester_123");
		networkInterface = Core::NetworkServerInterface::Create(port, messageQueue, sessions);
		INFO("Running on port {0}", port);
	}

	void ServerApp::OnEvent(Core::Event& e)
	{
		Application::OnEvent(e);

		Core::Event::Dispatch<Core::ConnectedEvent>(e, [this](Core::ConnectedEvent& e) { OnClientConnected(e); });
		Core::Event::Dispatch<Core::DisconnectedEvent>(e, [this](Core::DisconnectedEvent& e) { OnClientDisconnected(e); });
		Core::Event::Dispatch<Core::MessageSentEvent>(e, [this](Core::MessageSentEvent& e) { OnMessageSent(e); });
		Core::Event::Dispatch<Core::MessageAcceptedEvent>(e, [this](Core::MessageAcceptedEvent& e) { OnMessageAccepted(e); });
	}

	// Format (port: 20000)
	void ServerApp::ReadConfigFile()
	{
		std::ifstream file(configFilePath);

		std::string property;
		file >> property;

		if (property == "port:")
			file >> port;

		if (!port)
		{
			WriteConfigFile();
			ReadConfigFile();
		}
	}

	// Format (port: 20000)
	void ServerApp::WriteConfigFile()
	{
		std::ofstream file(configFilePath);

		// Write default port
		file << "port: " << 20000 << std::endl;
	}

	void ServerApp::OnClientConnected(Core::ConnectedEvent& e)
	{
		TRACE("New Connection: {0} on port {1}", e.GetDomain(), e.GetPort());
	}

	void ServerApp::OnClientDisconnected(Core::DisconnectedEvent& e)
	{
		TRACE("Client connection closed!");
	}

	void ServerApp::OnMessageSent(Core::MessageSentEvent& e)
	{
		TRACE("Message sent: {0} bytes to {1}", e.GetMessage().GetSize(), e.GetMessage().GetSessionId());
	}

	void ServerApp::OnMessageAccepted(Core::MessageAcceptedEvent& e)
	{
		TRACE("Message recieved: {0} bytes from {1}", e.GetMessage().GetSize(), e.GetMessage().GetSessionId());
	}

	void ServerApp::ProcessMessageQueue()
	{
		while (!databaseInterface->IsConnected())
		{
			networkInterface->DisconnectAllClients();
			databaseInterface->Reconnect();
		}

		for (uint32_t i = 0; i < messageQueue.GetCount(); i++)
			ProcessMessage();
	}

	void ServerApp::ProcessMessage()
	{
		Core::Message& message = messageQueue.Get();

		if (message.GetType() == Core::MessageType::Command)
		{
			Core::Command command;
			command.Deserialize(message.Body.Content);

			switch (command.GetType())
			{
				case Core::CommandType::Query:
				{
					databaseInterface->Query(command);

					Core::Response response(command.GetTaskId());
					databaseInterface->FetchData(response);

					SendResponse(response);

					break;
				}
				case Core::CommandType::Command:
				{
					bool success = databaseInterface->Execute(command);

					std::istringstream commandString(command.GetCommandString());
					if (command.GetTaskId())
					{
						Core::Response response(command.GetTaskId());
						response.AddData(new Core::DatabaseBool(success));

						if (success && (commandString.str().starts_with("INSERT") || commandString.str().starts_with("insert")));
						{
							Core::Command com;
							com.SetCommandString("SELECT LAST_INSERT_ID();");
							databaseInterface->Query(com);
							databaseInterface->FetchData(response);
						}
						SendResponse(response);
					}
					
					std::string tableName;
					for (uint32_t i = 0; i < 3; i++) // filter 3rd word (INSERT INTO table)
						commandString >> tableName;
					SendUpdateResponse(tableName);

					break;
				}
				case Core::CommandType::Update:
				{
					bool success = databaseInterface->Update(command);

					if (command.GetTaskId())
					{
						Core::Response response(command.GetTaskId());
						response.AddData(new Core::DatabaseBool(success));

						SendResponse(response);
					}
					
					std::istringstream commandString(command.GetCommandString());
					std::string tableName;
					for (uint32_t i = 0; i < 2; i++) // filter 2nd word (UPDATE table)
						commandString >> tableName;
					SendUpdateResponse(tableName);

					break;
				}
			}
		}

		messageQueue.Pop();
	}

	void ServerApp::SendResponse(Core::Response& response)
	{
		Core::Message& message = messageQueue.Get();

		Ref<Core::Message> responseMessaage = CreateRef<Core::Message>();
		responseMessaage->Header.Type = Core::MessageType::Response;
		responseMessaage->Header.SessionId = message.GetSessionId();

		response.Serialize(responseMessaage->Body.Content);
		responseMessaage->Header.Size = responseMessaage->Body.Content->GetSize();

		networkInterface->SendMessagePackets(responseMessaage);
	}

	void ServerApp::SendResponseToAllClients(Core::Response& response)
	{
		Core::Message& message = messageQueue.Get();

		Ref<Core::Message> responseMessaage = CreateRef<Core::Message>();
		responseMessaage->Header.Type = Core::MessageType::Response;

		response.Serialize(responseMessaage->Body.Content);
		responseMessaage->Header.Size = responseMessaage->Body.Content->GetSize();

		networkInterface->SendMessagePacketsToAllClients(responseMessaage);
	}

	void ServerApp::SendUpdateResponse(std::string& tableName)
	{
		if (tableName == "messages")
		{
			Core::Response response(6);
			SendResponseToAllClients(response);
		}
		else if (tableName == "users")
		{
			Core::Response response(7);
			SendResponseToAllClients(response);
		}
		else if (tableName == "teams")
		{
			Core::Response response(5);
			SendResponseToAllClients(response);
		}
		else if (tableName == "assignments" || tableName == "users_assignments")
		{
			Core::Response response(10);
			SendResponseToAllClients(response);
		}
		else if (tableName == "invites")
		{
			Core::Response response(8);
			SendResponseToAllClients(response);
		}
		else if (tableName == "notifications")
		{
			Core::Response response(9);
			SendResponseToAllClients(response);
		}
		else if (tableName == "users_teams")
		{
			{
				Core::Response response(5);
				SendResponseToAllClients(response);
			}

			{
				Core::Response response(7);
				SendResponseToAllClients(response);
			}

			{
				Core::Response response(8);
				SendResponseToAllClients(response);
			}

			{
				Core::Response response(9);
				SendResponseToAllClients(response);
			}
		}
	}
}

Core::Application* Core::CreateApplication(const Core::CommandArgs& args)
{
	Core::ApplicationSpecifications specs;
	specs.Title = "Server";
	specs.Args = args;
	specs.HasWindow = false;

	return new Server::ServerApp(specs);
}