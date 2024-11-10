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
	specs.Title = "Client";
	specs.Args = args;

	return new Client::ClientApp(specs);
}