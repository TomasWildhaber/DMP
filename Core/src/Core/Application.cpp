#include "pch.h"
#include "Application.h"
#include "Debugging/Log.h"
#include "GLFW/glfw3.h"

namespace Core
{
	Application::Application(const ApplicationSpecifications& _specs) : specs(_specs)
	{
		instance = this;

		TRACE("{0} application init", specs.Title);

		if (specs.HasWindow)
		{
			window = CreateRef<Window>(specs.WindowWidth, specs.WindowHeight, specs.Title);
			window->ShowWindow();
		}
	}

	void Application::Run()
	{
		TRACE("Works!");

		if (specs.HasWindow)
		{
			while (!glfwWindowShouldClose((GLFWwindow*)window->GetNativeWindow()))
			{
				window->OnUpdate();
				window->OnRender();
			}
		}
		else
			std::cin.get();
	}
}