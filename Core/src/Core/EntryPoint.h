#pragma once
#include "Application.h"

extern Core::Application* Core::CreateApplication(const Core::CommandArgs& args);

int Main(int argc, char** argv)
{
	Core::Application* app = Core::CreateApplication({ argc, argv });
	app->Run();
	delete app;

	return 0;
}

#ifdef PLATFORM_WINDOWS

#if !defined(DISTRIBUTION_CONFIG) || defined(SYSTEM_CONSOLE)
int main(int argc, char** argv)
{
	return Main(argc, argv);
}
#else
int WinMain(int argc, char** argv)
{
	return Main(argc, argv);
}
#endif

#endif