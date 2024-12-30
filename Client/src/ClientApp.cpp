#include "pch.h"
#include <imgui.h>
#include "ClientApp.h"
#include "Core/EntryPoint.h"
#include "Utils/Buffer.h"
#include "Debugging/Log.h"
#include "Resources/Fonts.h"


namespace Client
{
	ClientApp::ClientApp(const Core::ApplicationSpecifications& specs) : Core::Application(specs)
	{
		SetLoggerTitle("Client");

		networkInterface = Core::NetworkClientInterface::Create("127.0.0.1", 20000, messageQueue);
		window->SetRenderFunction([this]() { Render(); });
		SetStyle();

		if (state == ClientState::None)
			state = ClientState::Login;
	}

	void ClientApp::OnEvent(Core::Event& e)
	{
		Application::OnEvent(e);

		Core::Event::Dispatch<Core::ConnectedEvent>(e, [this](Core::ConnectedEvent& e) { OnConnect(e); });
		Core::Event::Dispatch<Core::DisconnectedEvent>(e, [this](Core::DisconnectedEvent& e) { OnDisconnect(e); });
		Core::Event::Dispatch<Core::MessageSentEvent>(e, [this](Core::MessageSentEvent& e) { OnMessageSent(e); });
		Core::Event::Dispatch<Core::MessageAcceptedEvent>(e, [this](Core::MessageAcceptedEvent& e) { OnMessageAccepted(e); });
	}

	void ClientApp::ProcessMessageQueue()
	{
		for (uint32_t i = 0; i < messageQueue.GetCount(); i++)
			ProcessMessage();
	}

	void ClientApp::ProcessMessage()
	{
		messageQueue.Pop();
	}

	void ClientApp::OnConnect(Core::ConnectedEvent& e)
	{
		INFO("Connected to {0} on port {1}", e.GetDomain(), e.GetPort());
	}

	void ClientApp::OnDisconnect(Core::DisconnectedEvent& e)
	{
		ERROR("Connection lost!");
	}

	void ClientApp::OnMessageSent(Core::MessageSentEvent& e)
	{
		INFO("Message sent: {0} bytes", e.GetMessage().GetSize());
	}

	void ClientApp::OnMessageAccepted(Core::MessageAcceptedEvent& e)
	{
		INFO("Message recieved: {0} bytes\n{1}", e.GetMessage().GetSize(), *e.GetMessage().Body.Content->GetDataAs<int>());
	}

	void ClientApp::Render()
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(io.DisplaySize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.396078431372549f, 0.3764705882352941f, 0.47058823529411764f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		ImGui::Begin("Background", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::End();

		ASSERT(state == ClientState::None, "Application state has not been set!");

		switch (state)
		{
			case Client::ClientState::Login:
			{
				ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
				ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

				ImGui::PushFont(fonts["Heading"]);
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Login").x) / 2);
				ImGui::Text("Login");
				ImGui::PopFont();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
				ImGui::PushFont(fonts["Regular"]);
				ImGui::Text("Don't have an account?");
				ImGui::SameLine();
				if (ImGui::TextLink("Create account"))
					state = ClientState::Register;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);

				static char emailBuffer[256] = { "Email" };
				ImGui::SetNextItemWidth(300.0f);
				ImGui::InputText("##Email", emailBuffer, sizeof(emailBuffer));

				static char passwordBuffer[256] = { "Password" };
				ImGui::SetNextItemWidth(300.0f);
				ImGui::InputText("##Password", passwordBuffer, sizeof(passwordBuffer));
				ImGui::PopStyleColor();
				ImGui::PopFont();

				ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Log in").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
				if (ImGui::Button("Log in") && networkInterface->IsConnected())
				{
					Ref<Core::Message> message = new Core::Message();
					message->Header.Size = 4;
					message->Body.Content = CreateRef<Buffer>(4);
					*message->Body.Content->GetDataAs<int>() = 32;

					networkInterface->SendMessagePackets(message);

					Ref<Core::Message> message2 = new Core::Message();
					message2->Header.Size = 4;
					message2->Body.Content = CreateRef<Buffer>(4);
					*message2->Body.Content->GetDataAs<int>() = 64;
					networkInterface->SendMessagePackets(message2);
				}
				ImGui::PopStyleVar();

				ImGui::End();

				break;
			}
			case Client::ClientState::Register:
			{
				ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
				ImGui::Begin("Create account", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

				ImGui::PushFont(fonts["Heading"]);
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Create an account").x) / 2);
				ImGui::Text("Create an account");
				ImGui::PopFont();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
				ImGui::PushFont(fonts["Regular"]);
				ImGui::Text("Already have an account?");
				ImGui::SameLine();
				if (ImGui::TextLink("Log in"))
					state = ClientState::Login;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
				ImGui::PopStyleVar();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 10.0f));
				static char codeBuffer[256] = { "Registration code" };
				ImGui::SetNextItemWidth(300.0f);
				ImGui::InputText("##Registration code", codeBuffer, sizeof(codeBuffer));

				static char firstNameBuffer[256] = { "First name" };
				ImGui::SetNextItemWidth(145.0f);
				ImGui::InputText("##First name", firstNameBuffer, sizeof(firstNameBuffer));

				ImGui::SameLine();
				static char lastNameBuffer[256] = { "Last name" };
				ImGui::SetNextItemWidth(145.0f);
				ImGui::InputText("##Last name", lastNameBuffer, sizeof(lastNameBuffer));

				static char emailBuffer[256] = { "Email" };
				ImGui::SetNextItemWidth(300.0f);
				ImGui::InputText("##Email", emailBuffer, sizeof(emailBuffer));

				static char passwordBuffer[256] = { "Password" };
				ImGui::SetNextItemWidth(300.0f);
				ImGui::InputText("##Password", passwordBuffer, sizeof(passwordBuffer));

				static char passwordCheckBuffer[256] = { "Confirm password" };
				ImGui::SetNextItemWidth(300.0f);
				ImGui::InputText("##Confirm password", passwordCheckBuffer, sizeof(passwordCheckBuffer));
				
				ImGui::PopStyleColor();
				ImGui::PopFont();

				ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Create account").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
				ImGui::Button("Create account");
				ImGui::PopStyleVar();

				ImGui::End();

				break;
			}
			case Client::ClientState::Home:
			{
				break;
			}
		}
	}

	void ClientApp::SetStyle()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		
		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;
		io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)&robotoMediumFontData, sizeof(robotoMediumFontData), 20.0f, &fontConfig);
		fonts["Heading"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoMediumFontData, sizeof(robotoMediumFontData), 38.0f, &fontConfig);
		fonts["Regular"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 16.0f, &fontConfig);

		io.ConfigWindowsMoveFromTitleBarOnly = true;

		style.WindowMenuButtonPosition = ImGuiDir_None;

		style.WindowBorderSize = 0.0f;
		style.WindowRounding = 8.0f;
		style.WindowPadding = ImVec2(20.0f, 20.0f);
		style.FrameBorderSize = 0.0f;
		style.FrameRounding = 5.0f;
		style.FramePadding = ImVec2(12.0f, 10.0f);

		style.Colors[ImGuiCol_WindowBg] = ImVec4{ 0.17862745098039217f ,0.16294117647058825f, 0.2296078431372549f, 1.0f };
		style.Colors[ImGuiCol_FrameBg] = ImVec4{ 0.23137254901960785f, 0.21176470588235294f, 0.2980392156862745f, 1.0f };
		style.Colors[ImGuiCol_Button] = ImVec4{ 0.43137254901960786f, 0.32941176470588235f, 0.7098039215686275f, 1.0f };
		style.Colors[ImGuiCol_ButtonHovered] = style.Colors[ImGuiCol_Button];
		style.Colors[ImGuiCol_ButtonActive] = ImVec4{ 0.33137254901960786f, 0.22941176470588235f, 0.6098039215686275f, 1.0f };
		style.Colors[ImGuiCol_Text] = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
		style.Colors[ImGuiCol_TextLink] = ImVec4{ 0.63137254901960786f, 0.52941176470588235f, 0.9098039215686275f, 1.0f };
	}
}

Core::Application* Core::CreateApplication(const Core::CommandArgs& args)
{
	Core::ApplicationSpecifications specs;
	specs.Args = args;
	specs.Title = "Client";
	specs.WindowWidth = 1280;
	specs.WindowHeight = 720;

	return new Client::ClientApp(specs);
}