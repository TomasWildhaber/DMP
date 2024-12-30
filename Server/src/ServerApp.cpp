#include "pch.h"
#include "Core/EntryPoint.h"
#include "Debugging/Log.h"
#include "ServerApp.h"

namespace Server
{
	ServerApp::ServerApp(const Core::ApplicationSpecifications& specs) : Core::Application(specs)
	{
		SetLoggerTitle("Server");

		networkInterface = Core::NetworkServerInterface::Create(20000, messageQueue, sessions);
	}

	void ServerApp::OnEvent(Core::Event& e)
	{
		Application::OnEvent(e);

		Core::Event::Dispatch<Core::ConnectedEvent>(e, [this](Core::ConnectedEvent& e) { OnClientConnected(e); });
		Core::Event::Dispatch<Core::DisconnectedEvent>(e, [this](Core::DisconnectedEvent& e) { OnClientDisconnected(e); });
		Core::Event::Dispatch<Core::MessageSentEvent>(e, [this](Core::MessageSentEvent& e) { OnMessageSent(e); });
		Core::Event::Dispatch<Core::MessageAcceptedEvent>(e, [this](Core::MessageAcceptedEvent& e) { OnMessageAccepted(e); });
	}

	void ServerApp::OnClientConnected(Core::ConnectedEvent& e)
	{
		INFO("New Connection: {0} on port {1}", e.GetDomain(), e.GetPort());
	}

	void ServerApp::OnClientDisconnected(Core::DisconnectedEvent& e)
	{
		INFO("Client connection closed!");
	}

	void ServerApp::OnMessageSent(Core::MessageSentEvent& e)
	{
		INFO("Message sent: {0} bytes to {1}", e.GetMessage().GetSize(), e.GetMessage().GetId());
	}

	void ServerApp::OnMessageAccepted(Core::MessageAcceptedEvent& e)
	{
		INFO("Message recieved: {0} bytes from {1}\n{2}", e.GetMessage().GetSize(), e.GetMessage().GetId(), *e.GetMessage().Body.Content->GetDataAs<int>());

		Ref<Core::Message> message = new Core::Message();
		message->Header.SessionId = 1;
		message->Header.Size = 4;
		message->Body.Content = CreateRef<Buffer>(4);
		*message->Body.Content->GetDataAs<int>() = 32;

		networkInterface->SendMessagePackets(message);
	}

	void ServerApp::ProcessMessageQueue()
	{
		for (uint32_t i = 0; i < messageQueue.GetCount(); i++)
			ProcessMessage();
	}

	void ServerApp::ProcessMessage()
	{
		messageQueue.Pop();
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