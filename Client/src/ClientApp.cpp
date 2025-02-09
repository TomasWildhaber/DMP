#include "pch.h"
#include <imgui.h>
#include "ClientApp.h"
#include "Core/EntryPoint.h"
#include "Utils/Buffer.h"
#include "Debugging/Log.h"
#include "Resources/Fonts.h"
#include "Database/Response.h"
#include "Database/Hash.h"

namespace Client
{
	// Global colors
	ImVec4 errorColor(0.845098039f, 0.0f, 0.0f, 1.0f);
	ImVec4 redButtonColor(0.5803921568627451f, 0.0f, 0.0f, 1.0f);
	ImVec4 redButtonActiveColor(0.4803921568627451f, 0.0f, 0.0f, 1.0f);
	ImVec4 successColor(0.09411764705882353f, 0.48627450980392156f, 0.09803921568627451f, 1.0f);

	// Global int to store id of invited user
	uint32_t invitedUserId = 0;

	// Global bools showing user page
	static bool showUserPage = false;

	ClientApp::ClientApp(const Core::ApplicationSpecifications& specs) : Core::Application(specs)
	{
		// Set logger title in dev mode
		SetLoggerTitle("Client");

		LoadConfig();
		networkInterface = Core::NetworkClientInterface::Create(address, port, messageQueue);
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

	void ClientApp::ReadConfigFile()
	{
		std::ifstream file(configFilePath);

		std::string property;
		while (file >> property)
		{
			if (property == "address:")
				file >> address;
			else if (property == "port:")
				file >> port;
			else
				break;
		}

		if (!port || address.empty())
		{
			WriteConfigFile();
			ReadConfigFile();
		}
	}

	void ClientApp::WriteConfigFile()
	{
		std::ofstream file(configFilePath);

		// Write default address and port
		file << "address: " << "127.0.0.1" << std::endl;
		file << "port: " << 20000 << std::endl;
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

			switch ((MessageResponses)response.GetTaskId())
			{
				case MessageResponses::Login: // Login
				{
					// Response: id, password hash, first name, last name, email, role
					if (response.HasData() && Core::ValidateHash(loginData.Password.c_str(), (const char*)response[1].GetValue()))
					{
						loggedUser.SetId(*(int*)response[0].GetValue());
						loggedUser.SetName(std::string((const char*)response[2].GetValue()) + " " + std::string((const char*)response[3].GetValue()));
						loggedUser.SetEmail((const char*)response[4].GetValue());

						if (std::string((const char*)response[5].GetValue()) == "user")
							loggedUser.SetAdminPrivileges(false);
						else
							loggedUser.SetAdminPrivileges(true);

						loginData.Error = LoginErrorType::None;
						state = ClientState::Home;

						ReadUsersTeams();
						ReadUsersInvites();
						ReadUsersNotifications();
					}
					else
						loginData.Error = LoginErrorType::Incorrect;

					loginData.Password.clear();
					break;
				}
				case MessageResponses::Register: // Register
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
				case MessageResponses::CheckEmail: // Check email
				{
					// Respsonse: id, email
					if (state == ClientState::Home)
					{
						if (!loggedUser.HasSelectedTeam() || !loggedUser.IsTeamOwner(loggedUser.GetSelectedTeam()))
							break;

						if (response.HasData())
						{
							invitedUserId = *(int*)response[0].GetValue();
							SendCheckInviteMessage(invitedUserId);
						}
						else
							inviteState = InviteState::NotExisting;
					}
					else
					{
						if (!response.HasData())
							SendRegisterMessage();
						else
							registerData.Error = RegisterErrorType::Existing;
					}

					break;
				}
				case MessageResponses::CheckInvite:
				{
					if (!loggedUser.HasSelectedTeam() || !loggedUser.IsTeamOwner(loggedUser.GetSelectedTeam()))
						break;

					if (!response.HasData())
						SendCheckTeamMessage(invitedUserId);
					else
						inviteState = InviteState::AlreadyInvited;

					break;
				}
				case MessageResponses::CheckTeam:
				{
					if (!loggedUser.HasSelectedTeam() || !loggedUser.IsTeamOwner(loggedUser.GetSelectedTeam()))
						break;

					if (!response.HasData())
					{
						inviteState = InviteState::InviteSuccessful;

						Core::Command command((uint32_t)MessageResponses::None);
						command.SetType(Core::CommandType::Command);

						command.SetCommandString("INSERT INTO invites (user_id, team_id) VALUES (?, ?);");
						command.AddData(new Core::DatabaseInt(invitedUserId));
						command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));

						SendCommandMessage(command);

						std::string message = "You have been invited to ";
						message += loggedUser.GetSelectedTeam().GetName();
						SendNotificationMessage(invitedUserId, message.c_str());
					}
					else
						inviteState = InviteState::AlreadyInTeam;

					break;
				}
				case MessageResponses::LinkTeamToUser: // Create user_teams relationship
				{
					if (*(bool*)response[0].GetValue())
					{
						int id = *(int*)response[1].GetValue();
						
						Core::Command command((uint32_t)MessageResponses::None);
						command.SetType(Core::CommandType::Command);

						command.SetCommandString("INSERT INTO users_teams (user_id, team_id) VALUES (?, ?);");
						command.AddData(new Core::DatabaseInt(loggedUser.GetId()));
						command.AddData(new Core::DatabaseInt(id));

						SendCommandMessage(command);
					}
					else
						ERROR("Team addition failed!");

					break;
				}
				case MessageResponses::UpdateTeams: // Update teams
				{
					ReadUsersTeams();

					break;
				}
				case MessageResponses::UpdateMessages: // Update messages
				{
					if (loggedUser.HasSelectedTeam())
						ReadSelectedTeamMessages();

					break;
				}
				case MessageResponses::UpdateUsers: // Update users
				{
					if (loggedUser.HasSelectedTeam())
						ReadSelectedTeamUsers();

					break;
				}
				case MessageResponses::UpdateInvites:
				{
					ReadUsersInvites();

					break;
				}
				case MessageResponses::UpdateNotifications:
				{
					ReadUsersNotifications();

					break;
				}
				case MessageResponses::ProcessTeams: // Process teams
				{
					loggedUser.ClearTeams();

					Ref<Team> firstTeam;
					// Load teams
					for (uint32_t i = 0; i < response.GetDataCount(); i += 3) // Response: id , owner_id, name
					{
						// Store first team refence
						if (i != 0)
							loggedUser.AddTeam(new Team(*(int*)response[i].GetValue(), *(int*)response[i + 1].GetValue(), (const char*)response[i + 2].GetValue())); // Add a team to team vector
						else
							firstTeam = loggedUser.AddTeam(new Team(*(int*)response[i].GetValue(), *(int*)response[i + 1].GetValue(), (const char*)response[i + 2].GetValue())); // Add a team to team vector and store it's reference
					}

					if (!loggedUser.IsSelectedTeamValid())
					{
						inviteState = InviteState::None;

						if (firstTeam && !showUserPage)
							loggedUser.SetSelectedTeam(firstTeam);
						else
							loggedUser.UnselectTeam();
					}

					// Read team messages and users if team is selected
					if (loggedUser.HasSelectedTeam())
					{
						ReadSelectedTeamMessages();
						ReadSelectedTeamUsers();
					}
					
					break;
				}
				case MessageResponses::ProcessTeamMessages: // Process team messages
				{
					if (loggedUser.HasSelectedTeam())
					{
						loggedUser.GetSelectedTeam().ClearMessages();

						// Load messages
						for (uint32_t i = 0; i < response.GetDataCount(); i += 4) // Response: id, content, first name, last name 
						{
							// Construct full name
							std::string name = std::string((const char*)response[i + 2].GetValue()) + " " + (const char*)response[i + 3].GetValue();
							// Add message
							loggedUser.GetSelectedTeam().AddMessage(new Message(name.c_str(), (const char*)response[i + 1].GetValue()));
						}
					}

					break;
				}
				case MessageResponses::ProcessTeamUsers:
				{
					if (loggedUser.HasSelectedTeam())
					{
						loggedUser.GetSelectedTeam().ClearUsers();

						// Load team users
						for (uint32_t i = 0; i < response.GetDataCount(); i += 3) // Response: id, first name, last name 
						{
							// Construct full name
							std::string name = std::string((const char*)response[i + 1].GetValue()) + " " + (const char*)response[i + 2].GetValue();
							// Add user
							loggedUser.GetSelectedTeam().AddUser(new User(*(int*)response[i].GetValue(), name));
						}
					}

					break;
				}
				case MessageResponses::ProcessInvites:
				{
					loggedUser.ClearInvites();

					// Load user invites
					for (uint32_t i = 0; i < response.GetDataCount(); i += 3) // Response: id, team id, team name
						loggedUser.AddInvite(new Invite(*(int*)response[i].GetValue(), *(int*)response[i + 1].GetValue(), (const char*)response[i + 2].GetValue())); // Add invite

					break;
				}
				case MessageResponses::ProcessNotifications:
				{
					loggedUser.ClearNotifications();

					// Load user notifications
					for (uint32_t i = 0; i < response.GetDataCount(); i += 2) // Response: id, message
						loggedUser.AddNotification(new Notification(*(int*)response[i].GetValue(), (const char*)response[i + 1].GetValue())); // Add notification

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
		// Set background to whole window size and it's color
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.396078431372549f, 0.3764705882352941f, 0.47058823529411764f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		// Render background
		ImGui::Begin("Background", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::End();

		// Ensure application state is set (in dev mode)
		ASSERT(state == ClientState::None, "Application state has not been set!");

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
		switch (state)
		{
			case Client::ClientState::Login:
			{
				RenderLoginWindow(windowFlags);
				break;
			}
			case Client::ClientState::Register:
			{
				RenderRegisterWindow(windowFlags);
				break;
			}
			case Client::ClientState::Home:
			{
				RenderHomeWindow(windowFlags);
				break;
			}
		}

		// If is not connected to server open modal window for reconnecting
		if (!networkInterface->IsConnected() && !ImGui::IsPopupOpen("Reconnect"))
			ImGui::OpenPopup("Reconnect");

		// Center reconnect popup if it's open
		if (ImGui::IsPopupOpen("Reconnect"))
			ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		// If is not connected to render open modal window for reconnecting
		if (ImGui::BeginPopupModal("Reconnect", nullptr, windowFlags))
		{
			ImGui::Text("Reconnecting...");

			// Close if is connected
			if (networkInterface->IsConnected())
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	void ClientApp::RenderLoginWindow(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		static char emailBuffer[256];
		static char passwordBuffer[256];

		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::Begin("Login", nullptr, flags);

		ImGui::PushFont(fonts["Heading"]);
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Login").x) / 2);
		ImGui::Text("Login");
		ImGui::PopFont();

		// Push error font and set position
		ImGui::PushFont(fonts["Error"]);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
		if (!(int)loginData.Error)
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 31.0f);

		// Render login error based on loginData.Error
		switch (loginData.Error)
		{
		case Client::LoginErrorType::NotFilled:
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("All inputs must be filled!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
			ImGui::TextColored(errorColor, "All inputs must be filled!");
			break;
		case Client::LoginErrorType::Incorrect:
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Email or password is incorrect!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
			ImGui::TextColored(errorColor, "Email or password is incorrect!");
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
			// Send login command if inputs are not empty otherwise set loginData.Error
			if (!std::string(emailBuffer).empty() && !std::string(passwordBuffer).empty())
			{
				loginData.Email = emailBuffer;
				loginData.Password = passwordBuffer;

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
				loginData.Password = passwordBuffer;

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

		// Switch application state and reset previous error if link is clicked
		if (ImGui::TextLink("Create account"))
		{
			loginData.Error = LoginErrorType::None;
			state = ClientState::Register;
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		ImGui::PopFont();

		ImGui::End();
	}

	void ClientApp::RenderRegisterWindow(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::Begin("Create account", nullptr, flags);

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
			ImGui::TextColored(errorColor, "All inputs must be filled!");
			break;
		case RegisterErrorType::NotMatching:
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Passwords does not match!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
			ImGui::TextColored(errorColor, "Passwords does not match!");
			break;
		case RegisterErrorType::Existing:
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Email already taken!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
			ImGui::TextColored(errorColor, "Email already taken!");
			break;
		case RegisterErrorType::WrongFormat:
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Email is in wrong format!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
			ImGui::TextColored(errorColor, "Email is in wrong format!");
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
				if (!strcmp(passwordBuffer, passwordCheckBuffer))
				{
					registerData.FirstName = firstNameBuffer;
					registerData.LastName = lastNameBuffer;
					registerData.Email = emailBuffer;
					registerData.PasswordHash = Core::GenerateHash(passwordBuffer);

					static std::regex pattern("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$");

					if (std::regex_match(registerData.Email, pattern))
					{
						SendCheckEmailMessage(registerData.Email.c_str());

						memset(firstNameBuffer, 0, sizeof(firstNameBuffer));
						memset(lastNameBuffer, 0, sizeof(lastNameBuffer));
						memset(emailBuffer, 0, sizeof(emailBuffer));
						memset(passwordBuffer, 0, sizeof(passwordBuffer));
						memset(passwordCheckBuffer, 0, sizeof(passwordCheckBuffer));
					}
					else
						registerData.Error = RegisterErrorType::WrongFormat;
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
		// Switch application state and reset previous error if link is clicked
		if (ImGui::TextLink("Log in"))
		{
			registerData.Error = RegisterErrorType::None;
			state = ClientState::Login;
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		ImGui::PopFont();

		ImGui::End();
	}

	void ClientApp::RenderHomeWindow(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Bools for rendering popups
		static bool openCreationPopup = false; // Popup for creating team after clicking on + button
		static bool openRenamePopup = false; // Popup for renaming team after clicking on Rename button
		static bool openDeletePopup = false; // Popup for deleting team after clicking on Delete button
		// Bool for automatic messages scroll
		static bool scrollDown = true;

		// Widths of side panels
		static float leftSidePanelWidth = 300.0f;
		static float rightSidePanelWidth = 400.0f;

		// Open creation popup
		if (openCreationPopup)
		{
			ImGui::OpenPopup("TeamCreation");
			openCreationPopup = false;
		}

		// Open rename popup
		if (openRenamePopup)
		{
			ImGui::OpenPopup("TeamRename");
			openRenamePopup = false;
		}

		// Open delete popup
		if (openDeletePopup)
		{
			ImGui::OpenPopup("TeamDelete");
			openDeletePopup = false;
		}

		// Center creation popup if it's open
		if (ImGui::IsPopupOpen("TeamCreation"))
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		// Render creation popup
		if (ImGui::BeginPopupModal("TeamCreation", nullptr, flags))
		{
			static char teamNameBuffer[26] = {}; // Limit team name to 25 characters

			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Create Team").x) / 2);
			ImGui::Text("Create Team");

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);
			ImGui::Text("Name");
			ImGui::SetNextItemWidth(260.0f);
			ImGui::InputText("##TeamName", teamNameBuffer, sizeof(teamNameBuffer));

			// Center buttons x position
			float buttonsWidth = ImGui::CalcTextSize("Cancel").x + ImGui::CalcTextSize("Create").x + ImGui::GetStyle().FramePadding.x * 2;
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonsWidth) / 2);

			if (ImGui::Button("Cancel"))
			{
				memset(teamNameBuffer, 0, sizeof(teamNameBuffer));
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Create") && !std::string(teamNameBuffer).empty())
			{
				Core::Command command((uint32_t)MessageResponses::LinkTeamToUser);
				command.SetType(Core::CommandType::Command);

				command.SetCommandString("INSERT INTO teams (name, owner_id) VALUES (?, ?);");
				command.AddData(new Core::DatabaseString(teamNameBuffer));
				command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

				SendCommandMessage(command);

				memset(teamNameBuffer, 0, sizeof(teamNameBuffer));
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		// Center rename popup if it's open
		if (ImGui::IsPopupOpen("TeamRename"))
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		// Render rename popup
		if (ImGui::BeginPopupModal("TeamRename", nullptr, flags))
		{
			static char teamNameBuffer[26] = {}; // Limit team name to 25 characters

			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Rename Team").x) / 2);
			ImGui::Text("Rename Team");

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);
			ImGui::Text("Name");
			ImGui::SetNextItemWidth(260.0f);
			ImGui::InputText("##TeamName", teamNameBuffer, sizeof(teamNameBuffer));

			// Center buttons x position
			float buttonsWidth = ImGui::CalcTextSize("Cancel").x + ImGui::CalcTextSize("Create").x + ImGui::GetStyle().FramePadding.x * 2;
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonsWidth) / 2);

			if (ImGui::Button("Cancel"))
			{
				memset(teamNameBuffer, 0, sizeof(teamNameBuffer));
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Rename"))
			{
				if (!std::string(teamNameBuffer).empty())
				{
					RenameSelectedTeam(teamNameBuffer);
					memset(teamNameBuffer, 0, sizeof(teamNameBuffer));
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}

		// Center delete popup if it's open
		if (ImGui::IsPopupOpen("TeamDelete"))
		{
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(260.0f, 0.0f));
		}

		// Render delete popup
		if (ImGui::BeginPopupModal("TeamDelete", nullptr, flags))
		{
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("You want to delete this team?").x) / 2);
			ImGui::Text("You want to delete this team?");

			// Center buttons x position
			float buttonsWidth = ImGui::CalcTextSize("No").x + ImGui::CalcTextSize("Yes").x + ImGui::GetStyle().FramePadding.x * 2;
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonsWidth) / 2);

			if (ImGui::Button("No"))
				ImGui::CloseCurrentPopup();

			ImGui::SameLine();
			if (ImGui::Button("Yes"))
			{
				DeleteSelectedTeam();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(leftSidePanelWidth, io.DisplaySize.y));
		ImGui::Begin("LeftSidePanel", nullptr, flags);
		ImGui::PopStyleVar();

		ImGui::BeginChild("Username", ImVec2(ImGui::GetWindowSize().x, 40.0f));
		ImGui::SetCursorPos(ImVec2(10.0f, 9.0f));
		ImGui::Text(loggedUser.GetName());

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
		{
			loggedUser.UnselectTeam();
			showUserPage = true;
		}

		ImGui::EndChild();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 8.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 1.0f));
		ImGui::SetCursorPosX(leftSidePanelWidth - 35.0f);
		if (ImGui::Button("+"))
			openCreationPopup = true;

		ImGui::PopStyleVar(2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
		ImGui::Indent(20.0f);
		for (auto& [id, team] : loggedUser.GetTeams())
		{
			ImGui::PushID(id);
			if (ImGui::Selectable(team->GetName(), loggedUser.HasSelectedTeam() && team->GetId() == loggedUser.GetSelectedTeam().GetId(), ImGuiSelectableFlags_SpanAllColumns))
			{
				showUserPage = false;
				scrollDown = true;
				inviteState = InviteState::None;
				loggedUser.SetSelectedTeam(team);
				ReadSelectedTeamMessages();
				ReadSelectedTeamUsers();
			}
			ImGui::PopID();
		}

		ImGui::PopStyleVar();
		ImGui::End();

		if (!loggedUser.HasTeams() && !showUserPage)
		{
			ImGui::SetNextWindowBgAlpha(0.0f);
			ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x + leftSidePanelWidth) / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::Begin("NoTeamsInfo", nullptr, flags);
			ImGui::TextColored(ImVec4(0.1f, 0.1f, 0.1f, 1.0f), "You have no teams yet");
			ImGui::End();
		}

		if (showUserPage)
		{
			ImGui::SetNextWindowBgAlpha(0.0f);
			ImGui::SetNextWindowPos(ImVec2(leftSidePanelWidth, 0.0f));
			ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - leftSidePanelWidth, io.DisplaySize.y));
			ImGui::Begin("UserPageWindow", nullptr, flags);

			ImGui::SetNextWindowBgAlpha(0.0f);
			ImGui::BeginChild("UserPageTabs", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2 - rightSidePanelWidth, ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y * 2));

			ImGui::BeginTabBar("UserPageTabBar");
			if (ImGui::BeginTabItem("Notifications"))
			{
				ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 90.0f);
				if (ImGui::Button("Clear all") && loggedUser.HasNotifications())
					DeleteAllNotifications();

				ImGui::BeginDisabled();
				for (Ref<Notification>& notification : loggedUser.GetNotifications())
				{
					const char* message = notification->GetMessage();

					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					ImGui::InputText("##Notification", (char*)message, strlen(message), ImGuiInputTextFlags_ReadOnly);
				}
				ImGui::EndDisabled();

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Invites"))
			{
				ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100.0f);
				if (ImGui::Button("Reject all") && loggedUser.HasInvites())
					DeleteAllInvites();

				for (Ref<Invite>& invite : loggedUser.GetInvites())
				{
					const char* teamName = invite->GetTeamName();

					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 170.0f);
					ImGui::BeginDisabled();
					ImGui::InputText("##Invite", (char*)teamName, strlen(teamName), ImGuiInputTextFlags_ReadOnly);
					ImGui::EndDisabled();
					ImGui::SameLine();
					if (ImGui::Button("Accept"))
					{
						Core::Command command((uint32_t)MessageResponses::None);
						command.SetType(Core::CommandType::Command);

						command.SetCommandString("INSERT INTO users_teams (user_id, team_id) VALUES (?, ?);");
						command.AddData(new Core::DatabaseInt(loggedUser.GetId()));
						command.AddData(new Core::DatabaseInt(invite->GetTeamId()));

						SendCommandMessage(command);

						DeleteInvite(invite->GetId());
					}

					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, redButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, redButtonActiveColor);
					if (ImGui::Button("Reject"))
						DeleteInvite(invite->GetId());

					ImGui::PopStyleColor(3);
				}

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();

			ImGui::EndChild();

			ImGui::SameLine();
			ImGui::Text("Change username");
			ImGui::End();
		}

		// Return if user has no selected team
		if (!loggedUser.HasSelectedTeam())
			return;

		// Render messages
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::SetNextWindowPos(ImVec2(leftSidePanelWidth, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - leftSidePanelWidth - rightSidePanelWidth, io.DisplaySize.y));
		ImGui::Begin("MessageWindow", nullptr, flags);

		ImGui::PushFont(fonts["Heading"]);
		ImGui::Text(loggedUser.GetSelectedTeam().GetName());
		ImGui::PopFont();

		// Render rename and delete buttons if user is owner of selected team
		if (loggedUser.IsTeamOwner(loggedUser.GetSelectedTeam()))
		{
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 190.0f);
			if (ImGui::Button("Rename"))
				openRenamePopup = true;

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, redButtonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redButtonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, redButtonActiveColor);

			if (ImGui::Button("Delete"))
				openDeletePopup = true;

			ImGui::PopStyleColor(3);
		}

		ImGui::Separator();

		ImGui::PushFont(fonts["Regular20"]);
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::BeginChild("Messages", ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2, ImGui::GetWindowHeight() - 160.0f), 0, ImGuiWindowFlags_NoScrollbar);

		if (!loggedUser.GetSelectedTeam().HasMessages())
		{
			ImVec2 textSize = ImGui::CalcTextSize("There are no messages yet");
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - textSize.x / 2, ImGui::GetWindowHeight() / 2 - textSize.y / 2));
			ImGui::TextColored(ImVec4(0.1f, 0.1f, 0.1f, 1.0f), "There are no messages yet");
		}

		for (uint32_t i = 0; i < loggedUser.GetSelectedTeam().GetMessageCount(); i++)
		{
			Ref<Message> message = loggedUser.GetSelectedTeam().GetMessages()[i];

			if (message->GetAuthorName() == loggedUser.GetName())
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

		// Auto scroll messages to buttom on load and after messages get updated
		if (scrollDown && loggedUser.GetSelectedTeam().GetMessageCount())
		{
			ImGui::SetScrollHereY();
			scrollDown = false;
		}

		ImGui::EndChild();

		static char messageBuffer[256] = {};
		ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2);
		ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 60.0f);
		if (ImGui::InputTextWithHint("##MessageInput", "Message...", messageBuffer, sizeof(messageBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			ImGui::SetKeyboardFocusHere(-1);
			scrollDown = true;

			Core::Command command((uint32_t)MessageResponses::None);
			command.SetType(Core::CommandType::Command);

			std::string message(messageBuffer);

			command.SetCommandString("INSERT INTO messages (content, team_id, author_id) VALUES (?, ?, ?)");
			command.AddData(new Core::DatabaseString(messageBuffer));
			command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));
			command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

			SendCommandMessage(command);

			memset(messageBuffer, 0, sizeof(messageBuffer));
		}

		ImGui::PopFont();
		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - rightSidePanelWidth, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(rightSidePanelWidth, io.DisplaySize.y));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::Begin("RightSidePanel", nullptr, flags);

		ImGui::BeginTabBar("TabBar");
		if (ImGui::BeginTabItem("Assignments"))
		{
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Members"))
		{
			static char emailBuffer[256] = {};

			bool IsOwner = loggedUser.IsTeamOwner(loggedUser.GetSelectedTeam());
			if (IsOwner)
			{
				ImGui::SetNextItemWidth(rightSidePanelWidth - (ImGui::GetStyle().WindowPadding.x * 2));
				ImGui::InputTextWithHint("##AddUser", "Email", emailBuffer, sizeof(emailBuffer));

				if (ImGui::Button("Add user"))
				{
					if (strcmp(emailBuffer, loggedUser.GetEmail()))
						SendCheckEmailMessage(emailBuffer);
					else
						inviteState = InviteState::CannotInviteYourself;

					memset(emailBuffer, 0, sizeof(emailBuffer));
				}

				switch (inviteState)
				{
				case Client::InviteState::None:
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 33.0f);
					break;
				case Client::InviteState::NotExisting:
					ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("User with this email does not exists!").x) / 2);
					ImGui::TextColored(errorColor, "User with this email does not exists!");
					break;
				case Client::InviteState::CannotInviteYourself:
					ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("You can't invite yourself!").x) / 2);
					ImGui::TextColored(errorColor, "You can't invite yourself!");
					break;
				case Client::InviteState::AlreadyInvited:
					ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("User has already been invited!").x) / 2);
					ImGui::TextColored(errorColor, "User has already been invited!");
					break;
				case Client::InviteState::AlreadyInTeam:
					ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("User is already in the team!").x) / 2);
					ImGui::TextColored(errorColor, "User is already in the team!");
					break;
				case Client::InviteState::InviteSuccessful:
					ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Invite has been sent!").x) / 2);
					ImGui::TextColored(successColor, "Invite has been sent!");
					break;
				}
			}

			for (Ref<User> teamUser : loggedUser.GetSelectedTeam().GetUsers())
			{
				ImGui::Text(teamUser->GetName());
				ImGui::AlignTextToFramePadding();

				if (IsOwner && !teamUser->IsTeamOwner(loggedUser.GetSelectedTeam()))
				{
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, redButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redButtonColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, redButtonActiveColor);

					if (ImGui::Button("Remove"))
					{
						Core::Command command((uint32_t)MessageResponses::None);
						command.SetType(Core::CommandType::Command);

						command.SetCommandString("DELETE FROM users_teams WHERE user_id = ? AND team_id = ?;");
						command.AddData(new Core::DatabaseInt(teamUser->GetId()));
						command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));
						SendCommandMessage(command);

						std::string message = "You have been removed from ";
						message += loggedUser.GetSelectedTeam().GetName();
						SendNotificationMessage(teamUser->GetId(), message.c_str());
					}

					ImGui::PopStyleColor(3);
				}
			}

			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void ClientApp::SetStyle()
	{
		// Get ImGui objects
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		
		// Allow specific characters
		ImVector<ImWchar> ranges;
		ImFontGlyphRangesBuilder builder;
		builder.AddText((const char*)u8"ášťřňčěžýíéóďúůäëïüöÁŠŤŘŇČĚŽÝÍÉÓĎŮÄËÏÖÜ");
		builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
		builder.BuildRanges(&ranges);

		// Load fonts
		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;
		io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)&robotoMediumFontData, sizeof(robotoMediumFontData), 20.0f, &fontConfig, ranges.Data);
		fonts["Heading"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoMediumFontData, sizeof(robotoMediumFontData), 38.0f, &fontConfig, ranges.Data);
		fonts["Regular"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 16.0f, &fontConfig, ranges.Data);
		fonts["Regular20"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 20.0f, &fontConfig, ranges.Data);
		fonts["Regular14"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 14.0f, &fontConfig, ranges.Data);
		fonts["Error"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 18.0f, &fontConfig, ranges.Data);
		io.Fonts->Build();

		// ImGui configuration
		io.ConfigWindowsMoveFromTitleBarOnly = true;
		io.LogFilename = nullptr;
		io.IniFilename = nullptr;

		// Set ImGui style
		style.WindowMenuButtonPosition = ImGuiDir_None;

		style.WindowBorderSize = 0.0f;
		style.WindowRounding = 8.0f;
		style.WindowPadding = ImVec2(20.0f, 20.0f);
		style.FrameBorderSize = 0.0f;
		style.FrameRounding = 5.0f;
		style.FramePadding = ImVec2(12.0f, 10.0f);
		style.ItemSpacing.y = 13.0f;
		style.DisabledAlpha = 0.9f;

		// Set ImGui Colors
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

		style.Colors[ImGuiCol_Tab] = style.Colors[ImGuiCol_Button];
		style.Colors[ImGuiCol_TabHovered] = style.Colors[ImGuiCol_ButtonActive];
		style.Colors[ImGuiCol_TabActive] = style.Colors[ImGuiCol_ButtonActive];
	}

	// Method to send command as message
	void ClientApp::SendCommandMessage(Core::Command& command)
	{
		Ref<Core::Message> message = CreateRef<Core::Message>();
		message->Header.Type = Core::MessageType::Command;
		command.Serialize(message->Body.Content);
		message->Header.Size = message->Body.Content->GetSize();

		networkInterface->SendMessagePackets(message);
	}

	void ClientApp::SendCheckEmailMessage(const char* email)
	{
		Core::Command command((uint32_t)MessageResponses::CheckEmail);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT id, email FROM users WHERE email = ?;");
		command.AddData(new Core::DatabaseString(email));

		SendCommandMessage(command);
	}

	void ClientApp::SendCheckInviteMessage(uint32_t userId)
	{
		Core::Command command((uint32_t)MessageResponses::CheckInvite);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT user_id FROM invites WHERE team_id = ? AND user_id = ?;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));
		command.AddData(new Core::DatabaseInt(userId));

		SendCommandMessage(command);
	}

	void ClientApp::SendCheckTeamMessage(uint32_t userId)
	{
		Core::Command command((uint32_t)MessageResponses::CheckTeam);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT user_id FROM users_teams WHERE team_id = ? AND user_id = ?;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));
		command.AddData(new Core::DatabaseInt(userId));

		SendCommandMessage(command);
	}

	// Send command to login
	void ClientApp::SendLoginMessage()
	{
		Core::Command command((uint32_t)MessageResponses::Login);
		command.SetType(Core::CommandType::Query);
		
		command.SetCommandString("SELECT id, password, first_name, last_name, email, role FROM users WHERE email=?;");
		command.AddData(new Core::DatabaseString(loginData.Email.c_str()));

		SendCommandMessage(command);
	}

	// Send command to register
	void ClientApp::SendRegisterMessage()
	{
		Core::Command command((uint32_t)MessageResponses::Register);
		command.SetType(Core::CommandType::Command);

		command.SetCommandString("INSERT INTO users (first_name, last_name, email, password) VALUES (?, ?, ?, ?);");
		command.AddData(new Core::DatabaseString(registerData.FirstName));
		command.AddData(new Core::DatabaseString(registerData.LastName));
		command.AddData(new Core::DatabaseString(registerData.Email));
		command.AddData(new Core::DatabaseString(registerData.PasswordHash));

		SendCommandMessage(command);
	}

	void ClientApp::SendNotificationMessage(uint32_t userId, const char* message)
	{
		Core::Command command((uint32_t)MessageResponses::None);
		command.SetType(Core::CommandType::Command);

		command.SetCommandString("INSERT INTO notifications (user_id, message) VALUES (?, ?);");
		command.AddData(new Core::DatabaseInt(userId));
		command.AddData(new Core::DatabaseString(message));
		SendCommandMessage(command);
	}

	// Send command to read all logged user's teams
	void ClientApp::ReadUsersTeams()
	{
		Core::Command command((uint32_t)MessageResponses::ProcessTeams);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT teams.id, teams.owner_id, teams.name FROM users_teams JOIN teams ON users_teams.team_id = teams.id WHERE users_teams.user_id = ?;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

		SendCommandMessage(command);
	}

	// Send command to read all logged user's invites
	void ClientApp::ReadUsersInvites()
	{
		Core::Command command((uint32_t)MessageResponses::ProcessInvites);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT invites.id, teams.id, teams.name FROM invites JOIN teams ON invites.team_id = teams.id WHERE invites.user_id = ?;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

		SendCommandMessage(command);
	}

	void ClientApp::ReadUsersNotifications()
	{
		Core::Command command((uint32_t)MessageResponses::ProcessNotifications);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT id, message FROM notifications WHERE user_id = ?;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

		SendCommandMessage(command);
	}

	// Send command to read all selected team messages
	void ClientApp::ReadSelectedTeamMessages()
	{
		Core::Command command((uint32_t)MessageResponses::ProcessTeamMessages);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT * FROM (SELECT messages.id, messages.content, users.first_name, users.last_name FROM messages JOIN users ON messages.author_id = users.id WHERE messages.team_id = ? ORDER BY messages.id DESC LIMIT 40) AS subquery ORDER BY subquery.id ASC;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));

		SendCommandMessage(command);
	}

	// Send command to read all selected team users
	void ClientApp::ReadSelectedTeamUsers()
	{
		Core::Command command((uint32_t)MessageResponses::ProcessTeamUsers);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT users.id, users.first_name, users.last_name FROM users_teams JOIN users ON users_teams.user_id = users.id WHERE users_teams.team_id = ?;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));

		SendCommandMessage(command);
	}

	void ClientApp::DeleteInvite(uint32_t inviteId)
	{
		Core::Command command((uint32_t)MessageResponses::None);
		command.SetType(Core::CommandType::Command);

		command.SetCommandString("DELETE FROM invites WHERE id = ?;");
		command.AddData(new Core::DatabaseInt(inviteId));

		SendCommandMessage(command);
	}

	void ClientApp::DeleteAllInvites()
	{
		Core::Command command((uint32_t)MessageResponses::None);
		command.SetType(Core::CommandType::Command);

		command.SetCommandString("DELETE FROM invites WHERE user_id = ?;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

		SendCommandMessage(command);
	}

	void ClientApp::DeleteAllNotifications()
	{
		Core::Command command((uint32_t)MessageResponses::None);
		command.SetType(Core::CommandType::Command);

		command.SetCommandString("DELETE FROM notifications WHERE user_id = ?;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

		SendCommandMessage(command);
	}

	// Send command to delete selected team
	void ClientApp::DeleteSelectedTeam()
	{
		Core::Command command((uint32_t)MessageResponses::None);
		command.SetType(Core::CommandType::Command);

		command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));
		command.SetCommandString("DELETE FROM users_teams WHERE team_id = ?;");
		SendCommandMessage(command);

		command.SetCommandString("DELETE FROM messages WHERE team_id = ?;");
		SendCommandMessage(command);

		command.SetCommandString("DELETE FROM invites WHERE team_id = ?;");
		SendCommandMessage(command);

		command.SetCommandString("DELETE FROM teams WHERE id = ?;");
		SendCommandMessage(command);
	}

	// Send command to rename selected team
	void ClientApp::RenameSelectedTeam(const char* teamName)
	{
		Core::Command command((uint32_t)MessageResponses::None);
		command.SetType(Core::CommandType::Update);

		command.SetCommandString("UPDATE teams set name = ? WHERE id = ?;");
		command.AddData(new Core::DatabaseString(teamName));
		command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));

		SendCommandMessage(command);
	}
}

// Set application specifications for ClientApp and return it's instance to Core
Core::Application* Core::CreateApplication(const Core::CommandArgs& args)
{
	Core::ApplicationSpecifications specs;
	specs.Args = args;
	specs.Title = "Client";
	specs.WindowWidth = 1280;
	specs.WindowHeight = 720;

	return new Client::ClientApp(specs);
}