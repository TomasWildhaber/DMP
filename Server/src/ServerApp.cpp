#include "ServerApp.h"

namespace Server
{
	ServerApp::ServerApp(const Core::ApplicationSpecifications& specs) : Core::Application(specs)
	{

	}
}

Core::Application* Core::CreateApplication(Core::CommandArgs args)
{
	Core::ApplicationSpecifications specs;
	specs.Title = "Server";
	specs.Args = args;

	return new Server::ServerApp(specs);
}