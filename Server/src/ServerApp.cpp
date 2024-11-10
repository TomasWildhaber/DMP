#include "pch.h"
#include "Core/EntryPoint.h"
#include "Debugging/Log.h"
#include "ServerApp.h"

namespace Server
{
	ServerApp::ServerApp(const Core::ApplicationSpecifications& specs) : Core::Application(specs)
	{
		SetLoggerTitle("Server");
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