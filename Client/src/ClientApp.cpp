﻿#include "pch.h"
#include <imgui.h>
#include "ClientApp.h"
#include "Core/EntryPoint.h"
#include "Utils/Buffer.h"
#include "Debugging/Log.h"
#include "Resources/Fonts.h"
#include "Database/Response.h"

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
		Core::Message& message = messageQueue.Get();

		if (message.GetType() == Core::MessageType::Response)
		{
			Core::Response response;
			response.Deserialize(message.Body.Content);

			switch (response.GetTaskId())
			{
				case 1: // Login
				{
					if (response.HasData() && std::string((const char*)response[1].GetValue()) == loginData.PasswordHash)
					{
						user.SetId(*(int*)response[0].GetValue());
						user.SetName(std::string((const char*)response[2].GetValue()) + " " + std::string((const char*)response[3].GetValue()));

						if (std::string((const char*)response[4].GetValue()) == "user")
							user.SetAdminPrivileges(false);
						else
							user.SetAdminPrivileges(true);

						ReadUserTeams();

						loginData.Error = LoginErrorType::None;
						state = ClientState::Home;
					}
					else
						loginData.Error = LoginErrorType::Incorrect;

					loginData.PasswordHash.clear();
					break;
				}
				case 2: // Register
				{
					if (*(bool*)response[0].GetValue())
					{
						registerData.Error = RegisterErrorType::None;
						state = ClientState::Login;
					}
					else
						ERROR("Register failed!");

					registerData.PasswordHash.clear();
					break;
				}
				case 3: // Check email
				{
					if (!response.HasData())
						SendRegisterMessage();
					else
						registerData.Error = RegisterErrorType::Existing;

					break;
				}
				case 4: // Create user_teams relationship
				{
					if (*(bool*)response[0].GetValue())
					{
						int id = *(int*)response[1].GetValue();
						
						Core::Command command(5);
						command.SetType(Core::CommandType::Command);

						command.SetCommandString("INSERT INTO users_teams (user_id, team_id) VALUES (?, ?);");
						command.AddData(new Core::DatabaseInt(user.GetId()));
						command.AddData(new Core::DatabaseInt(id));

						SendCommandMessage(command);
					}
					else
						ERROR("Team addition failed!");

					break;
				}
				case 5: // Update teams
				{
					ReadUserTeams();

					break;
				}
				case 6: // Process teams
				{
					user.ClearTeams();
					for (uint32_t i = 0; i < response.GetDataCount(); i += 2)
						user.AddTeam(new Team(*(int*)response[i].GetValue(), (const char*)response[i + 1].GetValue()));

					if (user.HasTeams())
						user.SetSelectedTeam(user.GetTeams()[0]);

					ReadTeamMessages();

					break;
				}
				case 8: // Process team messages
				{
					user.GetSelectedTeam().ClearMessages();
					for (uint32_t i = 0; i < response.GetDataCount(); i += 3)
					{
						std::string name = std::string((const char*)response[i + 1].GetValue()) + " " + (const char*)response[i + 2].GetValue();
						user.GetSelectedTeam().AddMessage(new Message(name.c_str(), (const char*)response[i].GetValue()));
					}

					break;
				}
			}
		}

		messageQueue.Pop();
	}

	void ClientApp::OnConnect(Core::ConnectedEvent& e)
	{
		TRACE("Connected to {0} on port {1}", e.GetDomain(), e.GetPort());
	}

	void ClientApp::OnDisconnect(Core::DisconnectedEvent& e)
	{
		ERROR("Connection lost!");
	}

	void ClientApp::OnMessageSent(Core::MessageSentEvent& e)
	{
		TRACE("Message sent: {0} bytes", e.GetMessage().GetSize());
	}

	void ClientApp::OnMessageAccepted(Core::MessageAcceptedEvent& e)
	{
		TRACE("Message recieved: {0} bytes", e.GetMessage().GetSize());
	}

	void ClientApp::Render()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;

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
				static char emailBuffer[256];
				static char passwordBuffer[256];

				ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
				ImGui::Begin("Login", nullptr, windowFlags);

				ImGui::PushFont(fonts["Heading"]);
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Login").x) / 2);
				ImGui::Text("Login");
				ImGui::PopFont();

				// Render login error
				ImGui::PushFont(fonts["Error"]);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
				if (!(int)loginData.Error)
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 31.0f);

				switch (loginData.Error)
				{
				case Client::LoginErrorType::NotFilled:
					ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("All inputs must be filled!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
					ImGui::TextColored(ImVec4(0.845098039f, 0.0f, 0.0f, 1.0f), "All inputs must be filled!");
					break;
				case Client::LoginErrorType::Incorrect:
					ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Email or password is incorrect!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
					ImGui::TextColored(ImVec4(0.845098039f, 0.0f, 0.0f, 1.0f), "Email or password is incorrect!");
					break;
				}

				ImGui::PopFont();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
				ImGui::PushFont(fonts["Regular"]);
				
				// Render text inputs
				ImGui::Text("Email");
				ImGui::SetNextItemWidth(300.0f);
				ImGui::InputText("##Email", emailBuffer, sizeof(emailBuffer));

				ImGui::Text("Password");
				ImGui::SetNextItemWidth(300.0f);
				if (ImGui::InputText("##Password", passwordBuffer, sizeof(passwordBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_Password))
				{
					if (!std::string(emailBuffer).empty() && !std::string(passwordBuffer).empty())
					{
						loginData.Email = emailBuffer;
						loginData.PasswordHash = passwordBuffer;

						SendLoginMessage();
					}
					else
						loginData.Error = LoginErrorType::NotFilled;
				}

				ImGui::PopStyleColor();
				ImGui::PopFont();

				// Render login button
				ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Log in").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
				if (ImGui::Button("Log in") && networkInterface->IsConnected())
				{
					if (!std::string(emailBuffer).empty() && !std::string(passwordBuffer).empty())
					{
						loginData.Email = emailBuffer;
						loginData.PasswordHash = passwordBuffer;

						SendLoginMessage();

						memset(emailBuffer, 0, sizeof(emailBuffer));
						memset(passwordBuffer, 0, sizeof(passwordBuffer));
					}
					else
						loginData.Error = LoginErrorType::NotFilled;
				}

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);

				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
				ImGui::PushFont(fonts["Regular"]);

				// Render register link
				ImGui::Text("Don't have an account?");
				ImGui::SameLine();

				if (ImGui::TextLink("Create account"))
					state = ClientState::Register;

				ImGui::PopStyleVar();
				ImGui::PopStyleColor();
				ImGui::PopFont();

				ImGui::End();

				break;
			}
			case Client::ClientState::Register:
			{
				ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
				ImGui::Begin("Create account", nullptr, windowFlags);

				ImGui::PushFont(fonts["Heading"]);
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Create an account").x) / 2);
				ImGui::Text("Create an account");
				ImGui::PopFont();

				// Render register error
				ImGui::PushFont(fonts["Error"]);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
				if (!(int)registerData.Error)
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 31.0f);

				switch (registerData.Error)
				{
				case RegisterErrorType::NotFilled:
					ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("All inputs must be filled!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
					ImGui::TextColored(ImVec4(0.845098039f, 0.0f, 0.0f, 1.0f), "All inputs must be filled!");
					break;
				case RegisterErrorType::NotMatching:
					ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Passwords does not match!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
					ImGui::TextColored(ImVec4(0.845098039f, 0.0f, 0.0f, 1.0f), "Passwords does not match!");
					break;
				case RegisterErrorType::Existing:
					ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Email already taken!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
					ImGui::TextColored(ImVec4(0.845098039f, 0.0f, 0.0f, 1.0f), "Email already taken!");
					break;
				}

				ImGui::PopFont();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
				ImGui::PushFont(fonts["Regular"]);
				
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
				ImGui::PopStyleVar();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 10.0f));

				// Render text inputs
				ImGui::Text("First name");
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 80.0f);
				ImGui::Text("Last name");

				static char firstNameBuffer[256];
				ImGui::SetNextItemWidth(145.0f);
				ImGui::InputText("##First name", firstNameBuffer, sizeof(firstNameBuffer));

				ImGui::SameLine();
				static char lastNameBuffer[256];
				ImGui::SetNextItemWidth(145.0f);
				ImGui::InputText("##Last name", lastNameBuffer, sizeof(lastNameBuffer));

				ImGui::Text("Email");
				static char emailBuffer[256];
				ImGui::SetNextItemWidth(300.0f);
				ImGui::InputText("##Email", emailBuffer, sizeof(emailBuffer));

				ImGui::Text("Password");
				static char passwordBuffer[256];
				ImGui::SetNextItemWidth(300.0f);
				ImGui::InputText("##Password", passwordBuffer, sizeof(passwordBuffer), ImGuiInputTextFlags_Password);

				ImGui::Text("Confirm password");
				static char passwordCheckBuffer[256];
				ImGui::SetNextItemWidth(300.0f);
				ImGui::InputText("##Confirm password", passwordCheckBuffer, sizeof(passwordCheckBuffer), ImGuiInputTextFlags_Password);
				
				ImGui::PopStyleColor();
				ImGui::PopFont();

				ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Create account").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
				if (ImGui::Button("Create account"))
				{
					if (!std::string(firstNameBuffer).empty() && !std::string(lastNameBuffer).empty() && !std::string(emailBuffer).empty() && !std::string(passwordBuffer).empty() && !std::string(passwordCheckBuffer).empty())
					{
						if (std::string(passwordBuffer) == std::string(passwordCheckBuffer))
						{
							registerData.FirstName = firstNameBuffer;
							registerData.LastName = lastNameBuffer;
							registerData.Email = emailBuffer;
							registerData.PasswordHash = passwordBuffer;

							Core::Command command(3);
							command.SetType(Core::CommandType::Query);

							command.SetCommandString("SELECT email FROM users WHERE email = ?;");
							command.AddData(new Core::DatabaseString(registerData.Email));

							SendCommandMessage(command);

							memset(firstNameBuffer, 0, sizeof(firstNameBuffer));
							memset(lastNameBuffer, 0, sizeof(lastNameBuffer));
							memset(emailBuffer, 0, sizeof(emailBuffer));
							memset(passwordBuffer, 0, sizeof(passwordBuffer));
							memset(passwordCheckBuffer, 0, sizeof(passwordCheckBuffer));
						}
						else
							registerData.Error = RegisterErrorType::NotMatching;
					}
					else
						registerData.Error = RegisterErrorType::NotFilled;
				}

				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
				ImGui::PushFont(fonts["Regular"]);

				ImGui::Text("Already have an account?");
				ImGui::SameLine();
				if (ImGui::TextLink("Log in"))
					state = ClientState::Login;

				ImGui::PopStyleVar();
				ImGui::PopStyleColor();
				ImGui::PopFont();

				ImGui::End();

				break;
			}
			case Client::ClientState::Home:
			{
				static bool openPopup = false;
				static float leftSidePanelSize = 300.0f;
				static float rightSidePanelSize = 300.0f;

				ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f));
				if (ImGui::IsPopupOpen("TeamCreation"))
					ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

				if (ImGui::BeginPopupModal("TeamCreation", nullptr, windowFlags))
				{
					static char teamNameBuffer[256] = {};

					ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Create Team").x) / 2);
					ImGui::Text("Create Team");

					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);
					ImGui::Text("Name");
					ImGui::SetNextItemWidth(260.0f);
					ImGui::InputText("##TeamName", teamNameBuffer, sizeof(teamNameBuffer));

					ImGui::SetCursorPos(ImVec2((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Create").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2, ImGui::GetCursorPosY() + 10.0f));
					if (ImGui::Button("Create"))
					{
						if (!std::string(teamNameBuffer).empty())
						{
							Core::Command command(4);
							command.SetType(Core::CommandType::Command);

							command.SetCommandString("INSERT INTO teams (name, owner_id) VALUES (?, ?);");
							command.AddData(new Core::DatabaseString(teamNameBuffer));
							command.AddData(new Core::DatabaseInt(user.GetId()));

							SendCommandMessage(command);

							memset(teamNameBuffer, 0, sizeof(teamNameBuffer));
						}

						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

				ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
				ImGui::SetNextWindowSize(ImVec2(leftSidePanelSize, io.DisplaySize.y));
				ImGui::Begin("LeftSidePanel", nullptr, windowFlags);
				ImGui::PopStyleVar();

				ImGui::BeginChild("Profile", ImVec2(ImGui::GetWindowSize().x, 40.0f));
				ImGui::SetCursorPos(ImVec2(10.0f, 9.0f));
				ImGui::Text(user.GetName());
				ImGui::EndChild();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 8.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 1.0f));
				ImGui::SetCursorPosX(leftSidePanelSize - 35.0f);
				if (ImGui::Button("+"))
					openPopup = true;

				ImGui::PopStyleVar(2);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
				ImGui::Indent(20.0f);
				for (uint32_t i = 0; i < user.GetTeamCount(); i++)
				{
					Ref<Team> team = user.GetTeams()[i];

					ImGui::PushID(i);
					if (ImGui::Selectable(team->GetName(), user.HasSelectedTeam() && team->GetId() == user.GetSelectedTeam().GetId(), ImGuiSelectableFlags_SpanAllColumns))
					{
						user.SetSelectedTeam(team);
						ReadTeamMessages();
					}
					ImGui::PopID();
				}

				ImGui::PopStyleVar();
				ImGui::End();

				if (!user.HasTeams())
				{
					ImGui::SetNextWindowBgAlpha(0.0f);
					ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x + leftSidePanelSize) / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
					ImGui::Begin("NoTeamsInfo", nullptr, windowFlags);
					ImGui::TextColored(ImVec4(0.1f, 0.1f, 0.1f, 1.0f), "You have no teams yet");
					ImGui::End();
				}

				if (openPopup)
				{
					ImGui::OpenPopup("TeamCreation");
					openPopup = false;
				}

				if (!user.HasSelectedTeam())
					break;

				ImGui::SetNextWindowBgAlpha(0.0f);
				ImGui::SetNextWindowPos(ImVec2(leftSidePanelSize, 0.0f));
				ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - leftSidePanelSize - rightSidePanelSize, io.DisplaySize.y));
				ImGui::Begin("MessageWindow", nullptr, windowFlags);

				ImGui::PushFont(fonts["Heading"]);
				ImGui::Text(user.GetSelectedTeam().GetName());
				ImGui::Separator();
				ImGui::PopFont();

				ImGui::PushFont(fonts["Regular20"]);
				ImGui::SetNextWindowBgAlpha(0.0f);
				ImGui::BeginChild("Messages", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2, ImGui::GetWindowHeight() - 150.0f), 0, ImGuiWindowFlags_NoScrollbar);

				if (!user.GetSelectedTeam().HasMessages())
				{
					ImVec2 textSize = ImGui::CalcTextSize("There are no messages yet");
					ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - textSize.x / 2, ImGui::GetWindowHeight() / 2 - textSize.y / 2));
					ImGui::TextColored(ImVec4(0.1f, 0.1f, 0.1f, 1.0f), "There are no messages yet");
				}

				for (uint32_t i = 0; i < user.GetSelectedTeam().GetMessageCount(); i++)
				{
					Ref<Message> message = user.GetSelectedTeam().GetMessages()[i];

					if (message->GetAuthorName() == user.GetName())
					{
						ImGui::PushFont(fonts["Regular14"]);

						float nameWidth = std::clamp(ImGui::CalcTextSize(message->GetAuthorName().c_str()).x, 0.0f, ImGui::GetWindowWidth() / 2);
						ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - nameWidth);

						ImGui::PushTextWrapPos(ImGui::GetWindowWidth());
						ImGui::TextWrapped(message->GetAuthorName().c_str());
						ImGui::PopFont();

						float messageWidth = std::clamp(ImGui::CalcTextSize(message->GetContent().c_str()).x, 0.0f, ImGui::GetWindowWidth() / 2);
						ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - messageWidth);
						ImGui::TextWrapped(message->GetContent().c_str());
						ImGui::PopTextWrapPos();
					}
					else
					{
						ImGui::PushFont(fonts["Regular14"]);
						ImGui::PushTextWrapPos(ImGui::GetWindowWidth() / 2);
						ImGui::TextWrapped(message->GetAuthorName().c_str());
						ImGui::PopFont();

						ImGui::TextWrapped(message->GetContent().c_str());
						ImGui::PopTextWrapPos();
					}
				}

				ImGui::EndChild();

				static char messageBuffer[256] = {};
				ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2);
				ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 60.0f);
				if (ImGui::InputText("##MessageInput", messageBuffer, sizeof(messageBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					Core::Command command(7);
					command.SetType(Core::CommandType::Command);

					std::string message(messageBuffer);

					command.SetCommandString("INSERT INTO messages (content, team_id, author_id) VALUES (?, ?, ?)");
					command.AddData(new Core::DatabaseString(messageBuffer));
					command.AddData(new Core::DatabaseInt(user.GetSelectedTeam().GetId()));
					command.AddData(new Core::DatabaseInt(user.GetId()));

					SendCommandMessage(command);

					memset(messageBuffer, 0, sizeof(messageBuffer));

					ReadTeamMessages();
				}

				ImGui::PopFont();
				ImGui::End();

				ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - rightSidePanelSize, 0.0f));
				ImGui::SetNextWindowSize(ImVec2(rightSidePanelSize, io.DisplaySize.y));

				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::Begin("RightSidePanel", nullptr, windowFlags);
				
				ImGui::BeginTabBar("TabBar");
				if (ImGui::BeginTabItem("Assignments"))
				{
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Members"))
				{
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();

				ImGui::End();
				ImGui::PopStyleVar();

				break;
			}
		}
	}

	void ClientApp::SetStyle()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		
		ImVector<ImWchar> ranges;
		ImFontGlyphRangesBuilder builder;
		builder.AddText((const char*)u8"ášťřňčěžýíéóďúůäëïüöÁŠŤŘŇČĚŽÝÍÉÓĎŮÄËÏÖÜ");
		builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
		builder.BuildRanges(&ranges);

		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;
		io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)&robotoMediumFontData, sizeof(robotoMediumFontData), 20.0f, &fontConfig, ranges.Data);
		fonts["Heading"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoMediumFontData, sizeof(robotoMediumFontData), 38.0f, &fontConfig, ranges.Data);
		fonts["Regular"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 16.0f, &fontConfig, ranges.Data);
		fonts["Regular20"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 20.0f, &fontConfig, ranges.Data);
		fonts["Regular14"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 14.0f, &fontConfig, ranges.Data);
		fonts["Error"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 18.0f, &fontConfig, ranges.Data);
		io.Fonts->Build();

		io.ConfigWindowsMoveFromTitleBarOnly = true;

		style.WindowMenuButtonPosition = ImGuiDir_None;

		style.WindowBorderSize = 0.0f;
		style.WindowRounding = 8.0f;
		style.WindowPadding = ImVec2(20.0f, 20.0f);
		style.FrameBorderSize = 0.0f;
		style.FrameRounding = 5.0f;
		style.FramePadding = ImVec2(12.0f, 10.0f);
		style.ItemSpacing.y = 13.0f;

		style.Colors[ImGuiCol_WindowBg] = ImVec4{ 0.17862745098039217f ,0.16294117647058825f, 0.2296078431372549f, 1.0f };
		style.Colors[ImGuiCol_ChildBg] = ImVec4 { 0.3764705882352941f ,0.3254901960784314f, 0.41568627450980394f, 1.0f };
		style.Colors[ImGuiCol_FrameBg] = ImVec4{ 0.23137254901960785f, 0.21176470588235294f, 0.2980392156862745f, 1.0f };
		style.Colors[ImGuiCol_Button] = ImVec4{ 0.43137254901960786f, 0.32941176470588235f, 0.7098039215686275f, 1.0f };
		style.Colors[ImGuiCol_ButtonHovered] = style.Colors[ImGuiCol_Button];
		style.Colors[ImGuiCol_ButtonActive] = ImVec4{ 0.33137254901960786f, 0.22941176470588235f, 0.6098039215686275f, 1.0f };
		style.Colors[ImGuiCol_Text] = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
		style.Colors[ImGuiCol_TextLink] = ImVec4{ 0.63137254901960786f, 0.52941176470588235f, 0.9098039215686275f, 1.0f };
		style.Colors[ImGuiCol_Header] = style.Colors[ImGuiCol_Button];
		style.Colors[ImGuiCol_HeaderHovered] = style.Colors[ImGuiCol_ButtonHovered];
		style.Colors[ImGuiCol_HeaderActive] = style.Colors[ImGuiCol_ButtonActive];
		style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_WindowBg];
	}

	void ClientApp::SendLoginMessage()
	{
		Core::Command command(1);
		command.SetType(Core::CommandType::Query);
		
		command.SetCommandString("SELECT id, password, first_name, last_name, role FROM users WHERE email=?;");
		command.AddData(new Core::DatabaseString(loginData.Email.c_str()));

		SendCommandMessage(command);
	}

	void ClientApp::SendRegisterMessage()
	{
		Core::Command command(2);
		command.SetType(Core::CommandType::Command);

		command.SetCommandString("INSERT INTO users (first_name, last_name, email, password, role) VALUES (?, ?, ?, ?, 'user');");
		command.AddData(new Core::DatabaseString(registerData.FirstName));
		command.AddData(new Core::DatabaseString(registerData.LastName));
		command.AddData(new Core::DatabaseString(registerData.Email));
		command.AddData(new Core::DatabaseString(registerData.PasswordHash));

		SendCommandMessage(command);
	}

	void ClientApp::SendCommandMessage(Core::Command& command)
	{
		Ref<Core::Message> message = CreateRef<Core::Message>();
		message->Header.Type = Core::MessageType::Command;
		command.Serialize(message->Body.Content);
		message->Header.Size = message->Body.Content->GetSize();

		networkInterface->SendMessagePackets(message);
	}

	void ClientApp::ReadUserTeams()
	{
		Core::Command command(6);
		command.SetType(Core::CommandType::Query);

		// TODO: select teams
		command.SetCommandString("SELECT teams.id, teams.name FROM users_teams JOIN teams ON users_teams.team_id = teams.id WHERE users_teams.user_id = ?;");
		command.AddData(new Core::DatabaseInt(user.GetId()));

		SendCommandMessage(command);
	}

	void ClientApp::ReadTeamMessages()
	{
		Core::Command command(8);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT messages.content, users.first_name, users.last_name FROM messages JOIN users ON messages.author_id = users.id JOIN teams ON messages.team_id = teams.id WHERE messages.team_id = ? ORDER BY messages.created_at ASC LIMIT 15;");
		command.AddData(new Core::DatabaseInt(user.GetSelectedTeam().GetId()));

		SendCommandMessage(command);
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