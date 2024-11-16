#include "pch.h"
#include "Window.h"
#include <Debugging/Log.h>
#include "Event/Events.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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
		setCallbacks();
		SetVSync(true);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos;
		io.DisplaySize = ImVec2((float)data.width, (float)data.height);

		ImGui_ImplOpenGL3_Init();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
	}

	Window::~Window()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Window::OnUpdate() const
	{
		glfwPollEvents();
	}

	void Window::OnRender() const
	{
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		data.renderFunction();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		ImGui::EndFrame();

		glfwSwapBuffers(window);
	}

	void Window::OnResize(WindowResizedEvent& e)
	{
		ImGui::GetIO().DisplaySize = ImVec2((float)data.width, (float)data.height);
	}

	void Window::setCallbacks()
	{
		glfwSetErrorCallback([](int error, const char* description) {
			ERROR("GLFW error {0}, {1}", error, description);
		});

		glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
			WindowData& _data = *(WindowData*)glfwGetWindowUserPointer(window);
			_data.width = width;
			_data.height = height;

			WindowResizedEvent event(width, height);
			_data.callbackFunction(event);
		});

		glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
			WindowData& _data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowClosedEvent event;
			_data.callbackFunction(event);
		});

		glfwSetWindowPosCallback(window, [](GLFWwindow* window, int x, int y) {
			WindowData& _data = *(WindowData*)glfwGetWindowUserPointer(window);
			_data.x = x;
			_data.y = y;

			WindowMovedEvent event(x, y);
			_data.callbackFunction(event);
		});

		glfwSetWindowFocusCallback(window, [](GLFWwindow* window, int focused) {
			WindowData& _data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowFocusedEvent event(focused);
			_data.callbackFunction(event);
		});

		glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			WindowData& _data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					_data.callbackFunction(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					_data.callbackFunction(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					_data.callbackFunction(event);
					break;
				}
			}
		});

		glfwSetCharCallback(window, [](GLFWwindow* window, uint32_t keyChar) {
			WindowData& _data = *(WindowData*)glfwGetWindowUserPointer(window);
			KeyTypedEvent event(keyChar);
			_data.callbackFunction(event);
		});

		glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
			WindowData& _data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					_data.callbackFunction(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					_data.callbackFunction(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
			WindowData& _data = *(WindowData*)glfwGetWindowUserPointer(window);
			MouseScrolledEvent event((float)xoffset, (float)yoffset);
			_data.callbackFunction(event);
		});

		glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xPos, double yPos) {
			WindowData& _data = *(WindowData*)glfwGetWindowUserPointer(window);
			MouseMovedEvent event((float)xPos, (float)yPos);
			_data.callbackFunction(event);
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