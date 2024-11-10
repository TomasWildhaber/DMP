#include "pch.h"
#include "Window.h"
#include "GLFW/glfw3.h"
#include <Debugging/Log.h>

namespace Core
{
	Window::Window(int Width, int Height, const char* Title)
	{
		data.width = Width;
		data.height = Height;
		ASSERT(!glfwInit(), "Failed to initialize glfw!");
		ASSERT(glfwGetCurrentContext() != nullptr, "Windows alredy created (multiple windows is not supported yet) => graphics API is initialized!");

		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		window = glfwCreateWindow(data.width, data.height, Title, nullptr, nullptr);
		glfwMakeContextCurrent(window);

		glfwSetWindowUserPointer(window, &data);
		SetVSync(true);
	}

	Window::~Window()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Window::OnUpdate() const
	{
		glfwPollEvents();
	}

	void Window::OnRender() const
	{
		glfwSwapBuffers(window);
	}

	void Window::setCallbacks()
	{
		glfwSetErrorCallback([](int error, const char* description) {
			ERROR("GLFW error {0}, {1}", error, description);
		});
	}

	inline void Window::SetVSync(bool Enabled)
	{
		glfwSwapInterval(Enabled);
		data.isVsync = Enabled;
	}

	inline bool Window::IsMaximized() const
	{
		glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
	}

	void Window::MaximizeWindow() const
	{
		glfwMaximizeWindow(window);
	}

	void Window::RestoreWindow() const
	{
		glfwRestoreWindow(window);
	}

	void Window::MinimizeWindow() const
	{
		glfwMaximizeWindow(window);
	}

	void Window::ShowWindow() const
	{
		glfwShowWindow(window);
	}

	inline void Window::HideWindow() const
	{
		glfwHideWindow(window);
	}
}