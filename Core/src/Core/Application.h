#pragma once

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
	};

	class Application
	{
	public:
		Application(const ApplicationSpecifications& _specs);

		Application(const Application& other) = delete;
		Application(const Application&& other) = delete;

		virtual ~Application() = default;

		void Run();

		static inline Application& Get() { return *instance; }
	private:
		ApplicationSpecifications specs;

		static inline Application* instance = nullptr;
	};

	Application* CreateApplication(CommandArgs args);
}