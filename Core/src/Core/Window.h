#pragma once
#include "Utils/Memory.h"
#include "Event/Event.h"
#include "Event/WindowEvent.h"

struct GLFWwindow;

namespace Core
{
	class Window
	{
		using EventCallbackFunction = std::function<void(Event&)>;
		using RenderFunction = std::function<void()>;
	public:
		Window(int Width, int Height, const char* Title);
		~Window();

		void OnUpdate() const;
		void OnRender() const;

		void SetRenderFunction(const RenderFunction& function) { data.renderFunction = function; }
		void SetEventCallbackFunction(const EventCallbackFunction& callback) { data.callbackFunction = callback; }

		inline int GetWidth() const { return data.width; }
		inline int GetHeight() const { return data.height; };
		inline std::pair<int, int> GetCurrentResolution() const { return { data.width, data.height }; }
		inline std::pair<int, int> GetCurrentPosition() const { return { data.x, data.y }; }
		inline void* GetNativeWindow() const { return (void*)window; }

		inline bool IsVSync() const { return data.isVsync; }
		inline bool IsMaximized() const;

		inline void SetVSync(bool Enabled);

		void MaximizeWindow() const;
		void RestoreWindow() const;
		void MinimizeWindow() const;

		void ShowWindow() const;
		void HideWindow() const;
		void CloseWindow() { WindowClosedEvent e; data.callbackFunction(e); }

		void OnResize(WindowResizedEvent& e);
	private:
		void setCallbacks();

		struct WindowData
		{
			int x, y, width, height;
			bool isVsync = false;
			EventCallbackFunction callbackFunction;
			RenderFunction renderFunction;
		};

		GLFWwindow* window;
		WindowData data;
	};
}