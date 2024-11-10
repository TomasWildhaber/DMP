#pragma once
#include "Utils/Memory.h"

struct GLFWwindow;

namespace Core
{
	class Window
	{
	public:
		Window(int Width, int Height, const char* Title);
		~Window();

		void OnUpdate() const;
		void OnRender() const;

		inline int GetWidth() const { return data.width; }
		inline int GetHeight() const { return data.height; };
		inline std::pair<int, int> GetCurrentResolution() const { return { data.width, data.height }; }
		inline std::pair<int, int> GetCurrentPosition() const { return { data.x, data.y }; }
		inline void* GetNativeWindow() const { return (void*)window; }

		inline void SetVSync(bool Enabled);
		inline bool IsVSync() const { return data.isVsync; }
		inline bool IsMaximized() const;

		void MaximizeWindow() const;
		void RestoreWindow() const;
		void MinimizeWindow() const;

		void ShowWindow() const;
		void HideWindow() const;
	private:
		void setCallbacks();

		struct WindowData
		{
			int x, y, width, height;
			bool isVsync = false;
		};

		WindowData data;
		GLFWwindow* window;
	};
}