#pragma once
#include "Utils/Memory.h"
#include "Window.h"
#include "Event/Events.h"

namespace Core
{
	struct CommandArgs
	{
		CommandArgs() = default;
		CommandArgs(int argc, char** argv) : Count(argc), Args(argv) {}

		int Count;
		char** Args;
	};

	struct ApplicationSpecifications
	{
		const char* Title;
		CommandArgs Args;
		bool HasWindow = true;
		int WindowWidth;
		int WindowHeight;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecifications& _specs);

		Application(const Application& other) = delete;
		Application(const Application&& other) = delete;

		virtual ~Application() = default;

		void LoadConfig();

		virtual void OnEvent(Event& e);
		void Run();

		static inline bool IsApplicationRunning() { return isApplicationRunning; }
		static inline Application& Get() { return *instance; }
		
		inline Window& GetWindow() { return window.Get(); }

		inline void Restart() { isRunning = false; }
	protected:
		virtual void ReadConfigFile() = 0;
		virtual void WriteConfigFile() = 0;

		void OnWindowClose(WindowClosedEvent& e);

		virtual void ProcessMessageQueue() = 0;

		std::string configFileName = "settings.cfg";
		std::filesystem::path configFilePath = std::filesystem::current_path() / configFileName;

		ApplicationSpecifications specs;
		Ref<Window> window;

		bool isRunning = true;

		static inline bool isApplicationRunning = true;
		static inline Application* instance = nullptr;
	};

	Application* CreateApplication(const CommandArgs& args);
}