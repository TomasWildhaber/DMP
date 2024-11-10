#include "pch.h"
#include "Core/EntryPoint.h"
#include "Debugging/Log.h"
#include "ClientApp.h"

namespace Client
{
	ClientApp::ClientApp(const Core::ApplicationSpecifications& specs) : Core::Application(specs)
	{
		SetLoggerTitle("Client");
	}
}

Core::Application* Core::CreateApplication(const Core::CommandArgs& args)
{
	Core::ApplicationSpecifications specs;
	specs.Args = args;
	specs.Title = "Client";
	specs.WindowWidth = 1280;
	specs.WindowHeight = 720;

	return new Client::ClientApp(specs);
}