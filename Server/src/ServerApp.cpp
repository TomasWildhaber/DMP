#include "pch.h"
#include "Core/EntryPoint.h"
#include "Debugging/Log.h"
#include "ServerApp.h"

#include "Database/Command.h"
#include "Database/Response.h"

#include "Utils/FileWriter.h"
#include "Utils/FileReader.h"

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
		else if (message.GetType() == Core::MessageType::ReadFileName)
		{
			uint32_t assignmentId = *message.Body.Content->GetDataAs<uint32_t>();

			Core::Command command;
			command.SetType(Core::CommandType::Query);

			command.SetCommandString("SELECT id, file_path, by_user FROM attachments WHERE assignment_id = ?;");
			command.AddData(new Core::DatabaseInt(assignmentId));
			databaseInterface->Query(command);

			Core::Response internResponse;
			databaseInterface->FetchData(internResponse);

			Core::Response response(11);
			for (uint32_t i = 0; i < internResponse.GetDataCount(); i += 3)
			{
				std::filesystem::path path = dir / (const char*)internResponse[i + 1].GetValue();
				Ref<Buffer> serializedData = FileReader::ReadFile(path);

				File file;
				file.DeserializeWithoutData(serializedData);
				response.AddData(new Core::DatabaseInt(file.GetId()));

				file.SetId(*(int*)internResponse[i].GetValue());
				response.AddData(new Core::DatabaseInt(file.GetId()));
				response.AddData(new Core::DatabaseString(file.GetName()));
				response.AddData(new Core::DatabaseBool(file.IsByUser()));
			}

			SendResponse(response);
		}
		else if (message.GetType() == Core::MessageType::DownloadFile)
		{
			uint32_t attachmentId = *message.Body.Content->GetDataAs<uint32_t>();

			Core::Command command;
			command.SetType(Core::CommandType::Query);

			command.SetCommandString("SELECT file_path FROM attachments WHERE id = ?;");
			command.AddData(new Core::DatabaseInt(attachmentId));
			databaseInterface->Query(command);

			Core::Response internResponse;
			databaseInterface->FetchData(internResponse);

			std::filesystem::path filePath = dir / (const char*)internResponse[0].GetValue();

			Ref<Buffer> serializedData = FileReader::ReadFile(filePath);

			File file;
			file.Deserialize(serializedData);

			SendFile(file);
		}
		else if (message.GetType() == Core::MessageType::UploadFile)
		{
			// Deserialize file name and assignment id
			File file;
			file.DeserializeWithoutData(message.Body.Content);

			if (!std::filesystem::exists(dir))
				std::filesystem::create_directories(dir);

			std::filesystem::path fileName = file.GetName();
			std::filesystem::path filePath = dir / fileName;

			// Resolve file name collisions
			int counter = 1;
			while (std::filesystem::exists(filePath))
			{
				filePath = dir / file.GetName();
				fileName = filePath.filename().string() + "(" + std::to_string(counter) + ")";
				filePath = dir / fileName;
				counter++;
			}

			// Create attachment in db
			Core::Command command;
			command.SetType(Core::CommandType::Command);

			command.SetCommandString("INSERT INTO attachments (assignment_id, file_path, by_user) VALUES (?, ?, ?);");
			command.AddData(new Core::DatabaseInt(file.GetId()));
			command.AddData(new Core::DatabaseString(fileName.string().c_str()));
			command.AddData(new Core::DatabaseBool(file.IsByUser()));
			databaseInterface->Execute(command);

			FileWriter::WriteFile(filePath, message.Body.Content);

			Core::Response response(10);
			SendResponseToAllClients(response);
		}

		messageQueue.Pop();
		
	#ifdef LOW_BANDWIDTH
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	#endif
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
		else if (tableName == "assignments" || tableName == "users_assignments" || tableName == "attachments")
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

	void ServerApp::SendFile(File& file)
	{
		Core::Message& message = messageQueue.Get();

		Ref<Core::Message> responseMessaage = CreateRef<Core::Message>();
		responseMessaage->Header.Type = Core::MessageType::DownloadFile;
		responseMessaage->Header.SessionId = message.GetSessionId();

		responseMessaage->Body.Content = file.GetData();
		responseMessaage->Header.Size = responseMessaage->Body.Content->GetSize();

		networkInterface->SendMessagePackets(responseMessaage);
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