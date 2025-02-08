#include "pch.h"
#include "Application.h"
#include "Debugging/Log.h"
#include <csignal>

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
		else
		{
			signal(SIGINT, [](int) {
				WindowClosedEvent event;
				Application::Get().OnEvent(event);
			});

			signal(SIGTERM, [](int) {
				WindowClosedEvent event;
				Application::Get().OnEvent(event);
			});
		}
	}

	void Application::LoadConfig()
	{
		if (!std::filesystem::exists(configFilePath))
			WriteConfigFile();

		ReadConfigFile();
	}

	void Application::OnEvent(Event& e)
	{
		Event::Dispatch<WindowResizedEvent>(e, [this](WindowResizedEvent& e) { window->OnResize(e); });
		Event::Dispatch<WindowClosedEvent>(e, [this](WindowClosedEvent& e) { OnWindowClose(e); });
	}

	void Application::Run()
	{
		while (isRunning)
		{
			ProcessMessageQueue();

			if (specs.HasWindow)
			{
				window->OnUpdate();
				window->OnRender();
			}
		}
	}

	void Application::OnWindowClose(WindowClosedEvent& e)
	{
		isRunning = false;
		isApplicationRunning = false;
	}
}