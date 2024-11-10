#include "pch.h"
#include "Application.h"
#include "Debugging/Log.h"

namespace Core
{
	Application::Application(const ApplicationSpecifications& _specs) : specs(_specs)
	{
		instance = this;

		TRACE("{0} application init", specs.Title);
	}

	void Application::Run()
	{
		TRACE("Works!");

		std::cin.get();
	}
}