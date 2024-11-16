#include "pch.h"
#include "Application.h"
#include "Debugging/Log.h"

namespace Core
{
	Application::Application(const ApplicationSpecifications& _specs) : specs(_specs)
	{
		instance = this;

		TRACE("{0} application init", specs.Title);

		if (specs.HasWindow)
		{
			window = CreateRef<Window>(specs.WindowWidth, specs.WindowHeight, specs.Title);
			window->SetEventCallbackFunction([this](Event& e) { OnEvent(e); });
			window->ShowWindow();
		}
	}

	void Application::OnEvent(Event& e)
	{
		Event::Dispatch<WindowClosedEvent>(e, [this](WindowClosedEvent& e) { OnWindowClose(e); });
	}

	void Application::Run()
	{
		TRACE("Works!");

		if (specs.HasWindow)
		{
			while (isRunning)
			{
				window->OnUpdate();
				window->OnRender();
			}
		}
		else
		{
			std::cin.get();
			isApplicationRunning = false;
		}
	}

	void Application::OnWindowClose(WindowClosedEvent& e)
	{
		isRunning = false;
		isApplicationRunning = false;
	}
}