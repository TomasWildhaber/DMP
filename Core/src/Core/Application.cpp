#include "Application.h"
#include <iostream>

namespace Core
{
	Application::Application(const ApplicationSpecifications& _specs) : specs(_specs)
	{
		instance = this;

		std::cout << specs.Title << " application init" << std::endl;
	}

	void Application::Run()
	{
		std::cout << "Works!" << std::endl;

		std::cin.get();
	}
}