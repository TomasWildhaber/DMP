#pragma once
#include "Application.h"

#ifdef PLATFORM_WINDOWS

extern Core::Application* Core::CreateApplication(Core::CommandArgs args);

int Main(int argc, char** argv)
{
	Core::Application* app = Core::CreateApplication({ argc, argv });
	app->Run();
	delete app;

	return 0;
}

#ifndef DISTRIBUTION_CONFIG
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