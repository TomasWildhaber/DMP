#include "ClientApp.h"

namespace Client
{
	ClientApp::ClientApp(const Core::ApplicationSpecifications& specs) : Core::Application(specs)
	{

	}
}

Core::Application* Core::CreateApplication(Core::CommandArgs args)
{
	Core::ApplicationSpecifications specs;
	specs.Title = "Client";
	specs.Args = args;

	return new Client::ClientApp(specs);
}