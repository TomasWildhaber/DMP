#include "pch.h"
#include <imgui.h>

#include <ImGuiDatePicker.hpp>
#include "ClientApp.h"
#include "Core/EntryPoint.h"
#include "Utils/Buffer.h"
#include "Debugging/Log.h"
#include "Resources/Fonts.h"
#include "Database/Response.h"
#include "Database/Hash.h"

#include "Utils/FileDialog.h"
#include "Utils/FileReader.h"
#include "Utils/FileWriter.h"

namespace Client
{
	const char* timeFormat = "%d.%m.%Y %H:%M"; // Format: 13.06.2025 23:59
	std::string downloadFileExtension;

	// Global colors
	ImVec4 errorColor(0.845098039f, 0.0f, 0.0f, 1.0f);
	ImVec4 redButtonColor(0.5803921568627451f, 0.0f, 0.0f, 1.0f);
	ImVec4 redButtonActiveColor(0.4803921568627451f, 0.0f, 0.0f, 1.0f);
	ImVec4 successColor(0.09411764705882353f, 0.48627450980392156f, 0.09803921568627451f, 1.0f);

	// Global bools showing user page
	bool showUserPage = false;

	// Global assignment data for assignment creation and edit
	AssignmentData editingAssignmentData;

	// Global int to store id of invited user
	uint32_t invitedUserId = 0;

	void SetFileExtension(const char* fileName)
	{
		const char* dot = strrchr(fileName, '.');
		if (!dot || dot == fileName)
			downloadFileExtension = "";

		downloadFileExtension = dot + 1;
	}

	ClientApp::ClientApp(const Core::ApplicationSpecifications& specs) : Core::Application(specs)
	{
		// Set logger title in dev mode
		SetLoggerTitle("Client");

		LoadConfig();
		networkInterface = Core::NetworkClientInterface::Create(address, port, messageQueue);
		window->SetRenderFunction([this]() { Render(); });
		SetStyle();

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
					if (response.HasData() && Core::ValidateHash(loginData.Password, response[1].GetValueCharPtr()))
					{
						loggedUser.SetId(response[0].GetValue<int>());
						loggedUser.SetName(std::string(response[2].GetValueCharPtr()) + " " + response[3].GetValueCharPtr());
						loggedUser.SetEmail(response[4].GetValueCharPtr());

						if (!strcmp(response[5].GetValueCharPtr(), "admin"))
							loggedUser.SetAdminPrivileges(true);
						else
							loggedUser.SetAdminPrivileges(false);

						loginData = LoginData();
						state = ClientState::Home;

						ReadUsersTeams();
						ReadUsersInvites();
						ReadUsersNotifications();
					}
					else
						loginData.Error = LoginErrorType::Incorrect;

					break;
				}
				case MessageResponses::Register: // Register
				{
					if (response[0].GetValue<bool>())
					{
						registerData = RegisterData();
						state = ClientState::Login;
					}
					else
						registerData.Error = RegisterErrorType::Error;

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
							invitedUserId = response[0].GetValue<int>();
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
				case MessageResponses::LinkTeamToUser: // Create users_teams relationship
				{
					if (response[0].GetValue<bool>())
					{
						int id = response[1].GetValue<int>();
						
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
				case MessageResponses::LinkAssignmentToUser: // Create users_assignments relationship and create it's attachments
				{
					if (response[0].GetValue<bool>())
					{
						int assignmentId = response[1].GetValue<int>();

						for (auto& [id, assignmentUser] : editingAssignmentData.GetUsers())
						{
							Core::Command command((uint32_t)MessageResponses::None);
							command.SetType(Core::CommandType::Command);

							command.SetCommandString("INSERT INTO users_assignments (user_id, assignment_id) VALUES (?, ?);");
							command.AddData(new Core::DatabaseInt(id));
							command.AddData(new Core::DatabaseInt(assignmentId));
							SendCommandMessage(command);
						}

						// Create attachments
						for (Ref<File>& attachment : editingAssignmentData.GetAttachments())
						{
							attachment->SetId(assignmentId);
							SendAttachment(attachment);
						}

						editingAssignmentData = AssignmentData();
					}
					else
						ERROR("Assignment addition failed!");

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
					if (state == ClientState::Home)
						UpdateLoggedUser();

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
				case MessageResponses::UpdateAssignments:
				{
					if (loggedUser.HasSelectedTeam())
						ReadAssignments();

					break;
				}
				case MessageResponses::UpdateLoggedUser:
				{
					loggedUser.SetName(std::string(response[0].GetValueCharPtr()) + " " + std::string(response[1].GetValueCharPtr()));
					loggedUser.SetEmail(response[2].GetValueCharPtr());

					if (std::string(response[3].GetValueCharPtr()) == "user")
						loggedUser.SetAdminPrivileges(false);
					else
						loggedUser.SetAdminPrivileges(true);

					break;
				}
				case MessageResponses::ProcessTeams: // Process teams
				{
					loggedUser.ClearTeams();

					Ref<Team> firstTeam;
					// Load teams
					for (uint32_t i = 0; i < response.GetDataCount(); i += 3) // Response: id , owner_id, name
					{
						// Store first team reference
						if (i != 0)
							loggedUser.AddTeam(new Team(response[i].GetValue<int>(), response[i + 1].GetValue<int>(), response[i + 2].GetValueCharPtr())); // Add a team to team vector
						else
							firstTeam = loggedUser.AddTeam(new Team(response[i].GetValue<int>(), response[i + 1].GetValue<int>(), response[i + 2].GetValueCharPtr())); // Add a team to team vector and store it's reference
					}

					if (!loggedUser.IsSelectedTeamValid())
					{
						ResetActionStates();

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
						ReadAssignments();
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
							std::string name = std::string(response[i + 2].GetValueCharPtr()) + " " + response[i + 3].GetValueCharPtr();
							// Add message
							loggedUser.GetSelectedTeam().AddMessage(new Message(name.c_str(), response[i + 1].GetValueCharPtr()));
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
							std::string name = std::string(response[i + 1].GetValueCharPtr()) + " " + response[i + 2].GetValueCharPtr();
							// Add user
							loggedUser.GetSelectedTeam().AddUser(new User(response[i].GetValue<int>(), name));
						}
					}

					break;
				}
				case MessageResponses::ProcessInvites:
				{
					loggedUser.ClearInvites();

					// Load user invites
					for (uint32_t i = 0; i < response.GetDataCount(); i += 3) // Response: id, team id, team name
						loggedUser.AddInvite(new Invite(response[i].GetValue<int>(), response[i + 1].GetValue<int>(), response[i + 2].GetValueCharPtr())); // Add invite

					break;
				}
				case MessageResponses::ProcessNotifications:
				{
					loggedUser.ClearNotifications();

					// Load user notifications
					for (uint32_t i = 0; i < response.GetDataCount(); i += 2) // Response: id, message
						loggedUser.AddNotification(new Notification(response[i].GetValue<int>(), response[i + 1].GetValueCharPtr())); // Add notification

					break;
				}
				case MessageResponses::ProcessAssignments:
				{
					loggedUser.ClearAssignments();

					for (uint32_t i = 0; i < response.GetDataCount(); i += 8) // Response: id, name, description, status, rating, rating_description, deadline, submitted_at
					{
						const char* dbStatus = response[i + 3].GetValueCharPtr();
						AssignmentStatus status;

						if (!strcmp(dbStatus, "in_progress"))
							status = AssignmentStatus::InProgress;
						else if (!strcmp(dbStatus, "submitted"))
							status = AssignmentStatus::Submitted;
						else
							status = AssignmentStatus::Rated;

						Ref<Assignment> assignment = new Assignment(response[i].GetValue<int>(), response[i + 1].GetValueCharPtr(), response[i + 2].GetValueCharPtr(), status, response[i + 4].GetValue<int>(), response[i + 5].GetValueCharPtr(), response[i + 6].GetValue<time_t>(), response[i + 7].GetValue<time_t>());
						loggedUser.AddAssignment(response[i].GetValue<int>(), assignment); // Add assignment
						ReadAssignmentsUsers(assignment); // Read assignment's users
						ReadAssignmentsAttachments(assignment); // Read assignment's attachments
					}

					break;
				}
				case MessageResponses::ProcessAssignmentsUsers:
				{
					if (response.HasData())
					{
						Ref<Assignment> assignment = loggedUser.GetAssignments()[response[0].GetValue<int>()];
						if (assignment)
						{
							assignment->ClearUsers();

							for (uint32_t i = 0; i < response.GetDataCount(); i += 4) // Response: assignment id, user id, first_name, last_name
							{
								// Construct full name
								std::string name = std::string(response[i + 2].GetValueCharPtr()) + " " + response[i + 3].GetValueCharPtr();
								// Add user to assignment
								Ref<User> user = new User(response[i + 1].GetValue<int>(), name);
								assignment->AddUser(user);
							}
						}
					}

					break;
				}
				case MessageResponses::ProcessAssignmentAttachments:
				{
					if (response.HasData())
					{
						Ref<Assignment> assignment = loggedUser.GetAssignments()[response[0].GetValue<int>()];
						if (assignment)
						{
							assignment->ClearAttachments();

							for (uint32_t i = 0; i < response.GetDataCount(); i += 4) // Response: assignment id, attachment id, file name, by_user
								assignment->AddAttachment(new File(response[i + 1].GetValue<int>(), response[i + 2].GetValueCharPtr(), response[i + 3].GetValue<bool>()));
						}
					}

					break;
				}
				case MessageResponses::ChangeUsername:
				{
					changeUsernameState = response[0].GetValue<int>() ? ChangeUsernameState::ChangeSuccessful : ChangeUsernameState::Error;

					break;
				}
				case MessageResponses::ChangePassword:
				{
					changePasswordState = response[0].GetValue<int>() ? ChangePasswordState::ChangeSuccessful : ChangePasswordState::Error;

					break;
				}
			}
		}
		else if (message.GetType() == Core::MessageType::DownloadFile)
		{
			auto path = FileDialog::SaveFile(downloadFileExtension.c_str());
			if (!path.empty())
				FileWriter::WriteFile(path, message.Body.Content);
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
		ImGui::InputText("##Email", loginData.Email, sizeof(loginData.Email));

		ImGui::Text("Password");
		ImGui::SetNextItemWidth(300.0f);
		// Login when enter pressed
		if (ImGui::InputText("##Password", loginData.Password, sizeof(loginData.Password), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_Password))
		{
			ImGui::SetKeyboardFocusHere(-1);

			// Send login command if inputs are not empty otherwise set loginData.Error
			if (strlen(loginData.Email) && strlen(loginData.Password))
				SendLoginMessage();
			else
				loginData.Error = LoginErrorType::NotFilled;
		}

		ImGui::PopStyleColor();
		ImGui::PopFont();

		// Render login button
		ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Log in").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
		// Login when login button clicked
		if (ImGui::Button("Log in") && networkInterface->IsConnected())
		{
			if (strlen(loginData.Email) && strlen(loginData.Password))
				SendLoginMessage();
			else
				loginData.Error = LoginErrorType::NotFilled;
		}

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
		ImGui::PushFont(fonts["Regular"]);

		// Render register link
		ImGui::Text("Don't have an account?");
		ImGui::SameLine();

		// Switch application state and reset login data if link is clicked
		if (ImGui::TextLink("Create account"))
		{
			loginData = LoginData();
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
		case RegisterErrorType::Error:
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Something went wrong!").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
			ImGui::TextColored(errorColor, "Something went wrong!");
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

		ImGui::SetNextItemWidth(145.0f);
		ImGui::InputText("##First name", registerData.FirstName, sizeof(registerData.FirstName));

		ImGui::SameLine();
		ImGui::SetNextItemWidth(145.0f);
		ImGui::InputText("##Last name", registerData.LastName, sizeof(registerData.LastName));

		ImGui::Text("Email");
		ImGui::SetNextItemWidth(300.0f);
		ImGui::InputText("##Email", registerData.Email, sizeof(registerData.Email));

		ImGui::Text("Password");
		ImGui::SetNextItemWidth(300.0f);
		ImGui::InputText("##Password", registerData.Password, sizeof(registerData.Password), ImGuiInputTextFlags_Password);

		ImGui::Text("Confirm password");
		ImGui::SetNextItemWidth(300.0f);
		ImGui::InputText("##Confirm password", registerData.CheckPassword, sizeof(registerData.CheckPassword), ImGuiInputTextFlags_Password);

		ImGui::PopStyleColor();
		ImGui::PopFont();

		ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Create account").x + ImGui::GetStyle().FramePadding.x * 2.0f) / 2);
		if (ImGui::Button("Create account"))
		{
			if (strlen(registerData.FirstName) && strlen(registerData.LastName) && strlen(registerData.Email) && strlen(registerData.Password) && strlen(registerData.CheckPassword))
			{
				if (!strcmp(registerData.Password, registerData.CheckPassword))
				{
					static std::regex pattern("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$");

					if (std::regex_match(registerData.Email, pattern))
						SendCheckEmailMessage(registerData.Email);
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
		// Switch application state and reset register data if link is clicked
		if (ImGui::TextLink("Log in"))
		{
			registerData = RegisterData();
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
		static bool openTeamCreationPopup = false; // Popup for creating team after clicking on + button
		static bool openRenameTeamPopup = false; // Popup for renaming team after clicking on Rename button
		static bool openDeleteTeamPopup = false; // Popup for deleting team after clicking on Delete button

		static bool openAssignmentCreationPopup = false; // Popup for creating assignment after clicking on New assignment button
		static bool openEditAssignmentPopup = false; // Popup for editing assignment after clicking on Edit button
		static bool openDeleteAssignmentPopup = false; // Popup for deleting assignment after clicking on Delete button
		static bool openRateAssignmentPopup = false; // Popup for rating assignment after clicking on Rate button
		// Bool for automatic messages scroll
		static bool scrollDown = true;

		// Open team creation popup
		if (openTeamCreationPopup)
		{
			ImGui::OpenPopup("TeamCreation");
			openTeamCreationPopup = false;
		}

		// Open rename team popup
		if (openRenameTeamPopup)
		{
			ImGui::OpenPopup("TeamRename");
			openRenameTeamPopup = false;
		}

		// Open delete team popup
		if (openDeleteTeamPopup)
		{
			ImGui::OpenPopup("TeamDelete");
			openDeleteTeamPopup = false;
		}

		// Open assignment creation popup
		if (openAssignmentCreationPopup)
		{
			ImGui::OpenPopup("AssignmentCreation");
			openAssignmentCreationPopup = false;
		}

		// Open delete assignment popup
		if (openDeleteAssignmentPopup)
		{
			ImGui::OpenPopup("AssignmentDelete");
			openDeleteAssignmentPopup = false;
		}

		// Open edit assignment popup
		if (openEditAssignmentPopup)
		{
			ImGui::OpenPopup("AssignmentEdit");
			openEditAssignmentPopup = false;
		}

		// Open rate assignment popup
		if (openRateAssignmentPopup)
		{
			ImGui::OpenPopup("AssignmentRate");
			openRateAssignmentPopup = false;
		}

		// Render all popup windows
		RenderCreateTeamPopup(flags);
		RenderRenameTeamPopup(flags);
		RenderDeleteTeamPopup(flags);

		RenderCreateAssignmentPopup(flags);
		RenderEditAssignmentPopup(flags);
		RenderDeleteAssignmentPopup(flags);
		RenderRateAssignmentPopup(flags);

		// Render Home page
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(leftSidePanelWidth, io.DisplaySize.y));
		ImGui::Begin("LeftSidePanel", nullptr, flags);
		ImGui::PopStyleVar();

		ImGui::BeginChild("Username", ImVec2(ImGui::GetWindowSize().x, 40.0f));
		ImGui::SetCursorPos(ImVec2(10.0f, 9.0f));
		ImGui::Text(loggedUser.GetName());
		
		// Render notification dot
		if (loggedUser.HasNotifications())
		{
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, redButtonColor);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 1.0f));

			ImGui::BeginDisabled();
			ImGui::Button(std::to_string(loggedUser.GetNotificationCount()).c_str());
			ImGui::EndDisabled();

			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}

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
			openTeamCreationPopup = true;

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
				ResetActionStates();

				loggedUser.SetSelectedTeam(team);
				ReadSelectedTeamMessages();
				ReadSelectedTeamUsers();
				ReadAssignments();
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
			RenderUserPage(flags);

		// Return if user has no selected team
		if (!loggedUser.HasSelectedTeam())
			return;

		//
		// Render selected team
		//

		bool isTeamOwner = loggedUser.IsTeamOwner(loggedUser.GetSelectedTeam());

		// Render messages
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::SetNextWindowPos(ImVec2(leftSidePanelWidth, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - leftSidePanelWidth - rightSidePanelWidth, io.DisplaySize.y));
		ImGui::Begin("MessageWindow", nullptr, flags);

		ImGui::PushFont(fonts["Heading"]);
		ImGui::Text(loggedUser.GetSelectedTeam().GetName());
		ImGui::PopFont();

		// Render rename and delete buttons if user is owner of selected team
		if (isTeamOwner)
		{
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 190.0f);
			if (ImGui::Button("Rename"))
				openRenameTeamPopup = true;

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, redButtonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redButtonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, redButtonActiveColor);

			if (ImGui::Button("Delete"))
				openDeleteTeamPopup = true;

			ImGui::PopStyleColor(3);
		}

		ImGui::Separator();

		ImGui::PushFont(fonts["Regular22"]);
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
				ImGui::PushFont(fonts["Regular16"]);

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
				ImGui::PushFont(fonts["Regular16"]);
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

		static char messageBuffer[CHAR_BUFFER_SIZE] = {};
		ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2);
		ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 65.0f);
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
			if (isTeamOwner)
			{
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("New assignment").x) / 2);
				if (ImGui::Button("New assignment"))
					openAssignmentCreationPopup = true;
			}

			ImVec2 padding(8.0f, 8.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
			if (ImGui::TreeNodeEx("Current assignments", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (auto& [id, assignment] : loggedUser.GetAssignments())
				{
					if (assignment->GetStatus() == AssignmentStatus::InProgress)
					{
						ImGui::PushID(assignment->GetId());
						bool assignmentOpened = ImGui::TreeNodeEx((void*)assignment->GetId(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_AllowItemOverlap, assignment->GetName());

						if (isTeamOwner)
						{
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding.x, padding.y - 3));

							int offset = 3.0f;
							ImGui::SameLine();
							ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionMax().x - 116.0f + offset, ImGui::GetCursorPosY() + offset));
							if (ImGui::Button("Edit"))
							{
								editingAssignmentData = assignment;
								openEditAssignmentPopup = true;
							}
							ImGui::SameLine();
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);

							ImGui::PushStyleColor(ImGuiCol_Button, redButtonColor);
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redButtonColor);
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, redButtonActiveColor);

							if (ImGui::Button("Delete"))
							{
								editingAssignmentData.AssignmentId = id;
								openDeleteAssignmentPopup = true;
							}

							ImGui::PopStyleVar();
							ImGui::PopStyleColor(3);
						}

						if (assignmentOpened)
						{
							RenderCommonAssignmentProperties(assignment);

							// Parse deadline to char array
							std::time_t deadline = assignment->GetDeadLine();
							std::tm* deadlineTime = localtime(&deadline);
							char deadlineStr[32];
							strftime(deadlineStr, sizeof(deadlineStr), timeFormat, deadlineTime);

							ImGui::Text("Deadline");
							ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

							bool isAfterDeadline = difftime(deadline, time(nullptr)) <= 0;
							if (isAfterDeadline)
								ImGui::PushStyleColor(ImGuiCol_Text, errorColor);

							ImGui::BeginDisabled();
							ImGui::InputText("##Deadline", deadlineStr, sizeof(deadlineStr));
							ImGui::EndDisabled();

							if (isAfterDeadline)
								ImGui::PopStyleColor();

							if (assignment->GetUserAttachmentCount())
							{
								ImGui::Text("Uploaded files");

								if (ImGui::BeginTable("UserAttachmentTable", 2))
								{
									bool shiftPressed = ImGui::IsKeyDown(ImGuiKey_LeftShift);

									ImGui::TableSetupColumn("AttachmentName", ImGuiTableColumnFlags_WidthStretch);
									ImGui::TableSetupColumn("RemoveButton", ImGuiTableColumnFlags_WidthFixed, !isTeamOwner && shiftPressed ? 80.0f : 100.0f);

									ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
									for (Ref<File> attachment : assignment->GetAttachments())
									{
										if (!attachment->IsByUser())
											continue;

										ImGui::TableNextColumn();
										ImGui::AlignTextToFramePadding();
										ImGui::Text(attachment->GetName());

										int attachmentId = attachment->GetId();
										ImGui::TableNextColumn();
										ImGui::PushID(attachmentId);

										if (isTeamOwner)
										{
											if (ImGui::Button("Download"))
											{
												SetFileExtension(attachment->GetName());
												DownloadAttachment(attachmentId);
											}
										}
										else
										{
											if (shiftPressed)
											{
												if (ImGui::Button("Remove"))
													DeleteAttachment(attachmentId);
											}
											else
											{
												if (ImGui::Button("Download"))
												{
													SetFileExtension(attachment->GetName());
													DownloadAttachment(attachmentId);
												}
											}
										}

										ImGui::PopID();
									}
									ImGui::PopStyleVar();

									ImGui::EndTable();
								}
							}

							if (!isTeamOwner)
							{
								ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Upload file").x) / 2);
								if (ImGui::Button("Upload file"))
								{
									auto filePath = FileDialog::OpenFile("");
									if (!filePath.empty())
									{
										Ref<Buffer> file = FileReader::ReadFile(filePath);

										if (file)
										{
											Ref<File> attachment = new File((const char*)filePath.filename().u8string().c_str(), file, true);
											attachment->SetId(assignment->GetId());

											SendAttachment(attachment);
										}
										else
											editingAssignmentData.Error = CreateAssignmentErrorType::MaxAttachmentSizeReached;
									}
								}

								if (ImGui::IsItemHovered())
									ImGui::SetTooltip("Maximum size of file can bo 20MB");

								ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Submit").x) / 2);
								if (ImGui::Button("Submit"))
								{
									Core::Command command((uint32_t)MessageResponses::None);
									command.SetType(Core::CommandType::Update);

									command.SetCommandString("UPDATE assignments set status = 'submitted', submitted_at = ? WHERE id = ?;");
									command.AddData(new Core::DatabaseTimestamp(time(nullptr)));
									command.AddData(new Core::DatabaseInt(assignment->GetId()));

									SendCommandMessage(command);

									std::string message = "Assignment ";
									message += assignment->GetName();
									message += " has been submitted";
									SendNotificationMessage(loggedUser.GetSelectedTeam().GetOwnerId(), message.c_str());
								}
							}
						}
						ImGui::PopID();
					}
				}

				ImGui::TreePop();
			}

			if (isTeamOwner && ImGui::TreeNodeEx("Submitted assignments", ImGuiTreeNodeFlags_Framed))
			{
				for (auto& [id, assignment] : loggedUser.GetAssignments())
				{
					if (assignment->GetStatus() == AssignmentStatus::Submitted)
					{
						ImGui::PushID(assignment->GetId());
						if (ImGui::TreeNodeEx((void*)assignment->GetId(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_AllowItemOverlap, assignment->GetName()))
						{
							RenderCommonAssignmentProperties(assignment);

							// Parse deadline to char array
							std::time_t deadline = assignment->GetDeadLine();
							std::tm* deadlineTime = localtime(&deadline);
							char deadlineStr[32];
							strftime(deadlineStr, sizeof(deadlineStr), timeFormat, deadlineTime);

							ImGui::Text("Deadline");
							ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
							ImGui::BeginDisabled();
							ImGui::InputText("##Deadline", deadlineStr, sizeof(deadlineStr));
							ImGui::EndDisabled();

							// Render submit time
							// Parse submmitTime to char array
							std::time_t submitTime = assignment->GetSubmitTime();
							std::tm* submitTimeTm = localtime(&submitTime);
							char submitTimeStr[32];
							strftime(submitTimeStr, sizeof(submitTimeStr), timeFormat, submitTimeTm);

							ImGui::Text("Submitted");
							ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

							bool submitAfterDeadline = difftime(submitTime, deadline) > 0;
							if (submitAfterDeadline)
								ImGui::PushStyleColor(ImGuiCol_Text, errorColor);

							ImGui::InputText("##SubmitTime", submitTimeStr, sizeof(submitTimeStr));

							if (submitAfterDeadline)
								ImGui::PopStyleColor();

							if (assignment->GetUserAttachmentCount())
							{
								ImGui::Text("Uploaded files");

								if (ImGui::BeginTable("UserAttachmentTable", 2))
								{
									ImGui::TableSetupColumn("AttachmentName", ImGuiTableColumnFlags_WidthStretch);
									ImGui::TableSetupColumn("RemoveButton", ImGuiTableColumnFlags_WidthFixed, 100.0f);

									ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
									for (Ref<File> attachment : assignment->GetAttachments())
									{
										if (!attachment->IsByUser())
											continue;

										ImGui::TableNextColumn();
										ImGui::AlignTextToFramePadding();
										ImGui::Text(attachment->GetName());

										int attachmentId = attachment->GetId();
										ImGui::TableNextColumn();
										ImGui::PushID(attachmentId);
										if (ImGui::Button("Download"))
										{
											SetFileExtension(attachment->GetName());
											DownloadAttachment(attachmentId);
										}
										ImGui::PopID();
									}
									ImGui::PopStyleVar();

									ImGui::EndTable();
								}
							}

							ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Rate").x) / 2);
							if (ImGui::Button("Rate"))
							{
								editingAssignmentData = assignment;
								openRateAssignmentPopup = true;
							}
						}
						ImGui::PopID();
					}
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Rated assignments", ImGuiTreeNodeFlags_Framed))
			{
				for (auto& [id, assignment] : loggedUser.GetAssignments())
				{
					if (assignment->GetStatus() == AssignmentStatus::Rated)
					{
						ImGui::PushID(assignment->GetId());
						if (ImGui::TreeNodeEx((void*)assignment->GetId(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_AllowItemOverlap, assignment->GetName()))
						{
							RenderCommonAssignmentProperties(assignment);

							// Render deadline
							// Parse deadline to char array
							std::time_t deadline = assignment->GetDeadLine();
							std::tm* deadlineTime = localtime(&deadline);
							char deadlineStr[32];
							strftime(deadlineStr, sizeof(deadlineStr), timeFormat, deadlineTime);

							ImGui::Text("Deadline");
							ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
							ImGui::BeginDisabled();
							ImGui::InputText("##Deadline", deadlineStr, sizeof(deadlineStr));
							ImGui::EndDisabled();

							// Render submit time
							// Parse submmitTime to char array
							std::time_t submitTime = assignment->GetSubmitTime();
							std::tm* submitTimeTm = localtime(&submitTime);
							char submitTimeStr[32];
							strftime(submitTimeStr, sizeof(submitTimeStr), timeFormat, submitTimeTm);

							ImGui::Text("Submitted");
							ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

							bool submitAfterDeadline = difftime(submitTime, deadline) > 0;
							if (submitAfterDeadline)
								ImGui::PushStyleColor(ImGuiCol_Text, errorColor);

							ImGui::InputText("##SubmitTime", submitTimeStr, sizeof(submitTimeStr));

							if (submitAfterDeadline)
								ImGui::PopStyleColor();

							if (assignment->GetUserAttachmentCount())
							{
								ImGui::Text("Uploaded files");

								if (ImGui::BeginTable("UserAttachmentTable", 2))
								{
									ImGui::TableSetupColumn("AttachmentName", ImGuiTableColumnFlags_WidthStretch);
									ImGui::TableSetupColumn("RemoveButton", ImGuiTableColumnFlags_WidthFixed, 100.0f);

									ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
									for (Ref<File> attachment : assignment->GetAttachments())
									{
										if (!attachment->IsByUser())
											continue;

										ImGui::TableNextColumn();
										ImGui::AlignTextToFramePadding();
										ImGui::Text(attachment->GetName());

										int attachmentId = attachment->GetId();
										ImGui::TableNextColumn();
										ImGui::PushID(attachmentId);
										if (ImGui::Button("Download"))
										{
											SetFileExtension(attachment->GetName());
											DownloadAttachment(attachmentId);
										}
										ImGui::PopID();
									}
									ImGui::PopStyleVar();

									ImGui::EndTable();
								}
							}

							// Render rating
							ImGui::BeginDisabled();
							if (assignment->GetRatingDescriptionSize())
							{
								ImGui::Text("Rated description");
								ImGui::InputTextMultiline("##rating_desc", (char*)assignment->GetRatingDescription(), assignment->GetDescriptionSize(), ImVec2(ImGui::GetContentRegionAvail().x, 100.0f));
							}

							std::string rating = "Rating: " + std::to_string(assignment->GetRating());
							ImGui::Text(rating.c_str());

							ImGui::EndDisabled();
						}
						ImGui::PopID();
					}
				}

				ImGui::TreePop();
			}
			ImGui::PopStyleVar();

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Members"))
		{
			static char emailBuffer[CHAR_BUFFER_SIZE] = {};

			if (isTeamOwner)
			{
				ImGui::SetNextItemWidth(rightSidePanelWidth - (ImGui::GetStyle().WindowPadding.x * 2));
				ImGui::InputTextWithHint("##AddUser", "Email", emailBuffer, sizeof(emailBuffer));

				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Add user").x) / 2);
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
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 33.0f); // Skip height of the error text height
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

			ImGui::SetNextWindowBgAlpha(0.0f);
			if (ImGui::BeginTable("MemberTable", 2, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
			{
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("RemoveButton", ImGuiTableColumnFlags_WidthFixed, 100.0f);
				
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
				for (Ref<User> teamUser : loggedUser.GetSelectedTeam().GetUsers())
				{
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text(teamUser->GetName());
					ImGui::TableNextColumn();

					if (isTeamOwner && !teamUser->IsTeamOwner(loggedUser.GetSelectedTeam()))
					{
						ImGui::SameLine();
						ImGui::PushStyleColor(ImGuiCol_Button, redButtonColor);
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redButtonColor);
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, redButtonActiveColor);

						ImGui::PushID(teamUser->GetId());
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
						ImGui::PopID();

						ImGui::PopStyleColor(3);
					}
				}
				ImGui::PopStyleVar();

				ImGui::EndTable();
			}
			
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void ClientApp::RenderCreateTeamPopup(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Center team creation popup if it's open
		if (ImGui::IsPopupOpen("TeamCreation"))
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		// Render team creation popup
		if (ImGui::BeginPopupModal("TeamCreation", nullptr, flags))
		{
			static char teamNameBuffer[CHAR_SHORT_BUFFER_SIZE] = {}; // Limit team name to 25 characters

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
	}

	void ClientApp::RenderRenameTeamPopup(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Center rename popup if it's open
		if (ImGui::IsPopupOpen("TeamRename"))
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		// Render rename popup
		if (ImGui::BeginPopupModal("TeamRename", nullptr, flags))
		{
			static char teamNameBuffer[CHAR_SHORT_BUFFER_SIZE] = {}; // Limit team name to 25 characters

			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Rename Team").x) / 2);
			ImGui::Text("Rename Team");

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);
			ImGui::Text("Name");
			ImGui::SetNextItemWidth(260.0f);
			ImGui::InputText("##TeamName", teamNameBuffer, sizeof(teamNameBuffer));

			// Center buttons x position
			float buttonsWidth = ImGui::CalcTextSize("Cancel").x + ImGui::CalcTextSize("Rename").x + ImGui::GetStyle().FramePadding.x * 2;
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonsWidth) / 2);

			if (ImGui::Button("Cancel"))
			{
				memset(teamNameBuffer, 0, sizeof(teamNameBuffer));
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Rename"))
			{
				if (strlen(teamNameBuffer))
				{
					RenameSelectedTeam(teamNameBuffer);
					memset(teamNameBuffer, 0, sizeof(teamNameBuffer));
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}
	}

	void ClientApp::RenderDeleteTeamPopup(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Center delete team popup if it's open
		if (ImGui::IsPopupOpen("TeamDelete"))
		{
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(260.0f, 0.0f));
		}

		// Render delete team popup
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
	}

	void ClientApp::RenderCreateAssignmentPopup(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Center assignment creation popup if it's open
		if (ImGui::IsPopupOpen("AssignmentCreation"))
		{
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(400.0f, 0.0f));
		}

		// Render assignment creation popup
		if (ImGui::BeginPopupModal("AssignmentCreation", nullptr, flags))
		{
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Create assignment").x) / 2);
			ImGui::Text("Create assignment");

			ImGui::Text("Name");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputText("##Name", editingAssignmentData.Name, editingAssignmentData.GetNameSize());

			ImGui::Text("Task");
			ImGui::InputTextMultiline("##TaskInput", editingAssignmentData.Description, editingAssignmentData.GetDescriptionSize(), ImVec2(ImGui::GetContentRegionAvail().x, 100.0f));

			// Render Add attachment button
			if (ImGui::Button("Add attachment"))
			{
				auto filePath = FileDialog::OpenFile("");
				if (!filePath.empty())
				{
					Ref<Buffer> file = FileReader::ReadFile(filePath);

					if (file)
						editingAssignmentData.AddAttachment(new File((const char*)filePath.filename().u8string().c_str(), file, false));
					else
						editingAssignmentData.Error = CreateAssignmentErrorType::MaxAttachmentSizeReached;
				}
			}

			// Tool tip when Add attachment button is hovered
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Maximum size of file can be 20MB");

			// Render attachments in assignment
			if (editingAssignmentData.GetAttachmentCount())
				ImGui::Text("Attachments");

			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redButtonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, redButtonActiveColor);

			for (uint32_t j = 0; j < editingAssignmentData.GetAttachmentCount(); j++)
			{
				Ref<File>& attachment = editingAssignmentData.GetAttachments()[j];

				// Calculate button wrapping
				float windowRightBorder = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
				float buttonSize = ImGui::CalcItemWidth();
				float lastButtonPos = ImGui::GetItemRectMin().x;
				float nextButtonPos = lastButtonPos + ImGui::GetStyle().ItemSpacing.x + buttonSize;

				if (j != 0 && nextButtonPos < windowRightBorder)
					ImGui::SameLine();

				// Remove clicked attachment
				if (ImGui::Button(attachment->GetName()))
				{
					editingAssignmentData.RemoveAttachment(j);
					j--;
				}
			}
			ImGui::PopStyleColor(2);

			ImGui::Text("Users");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::BeginCombo("##AddUserCombo", nullptr))
			{
				int i = 0;
				for (const Ref<User>& teamUser : loggedUser.GetSelectedTeam().GetUsers())
				{
					if (i++ == 0) // Remove team owner form list of addable users
						continue;

					ImGui::PushID(teamUser->GetId());
					if (ImGui::Selectable(teamUser->GetName()))
						editingAssignmentData.AddUser(teamUser);
					ImGui::PopID();
				}

				ImGui::EndCombo();
			}

			int removedUserId = -1;
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, redButtonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, redButtonActiveColor);

			// Render users in assignment
			uint32_t i = 0;
			for (auto& [id, assignmentUser] : editingAssignmentData.GetUsers())
			{
				float windowRightBorder = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
				float buttonSize = ImGui::CalcItemWidth();
				float lastButtonPos = ImGui::GetItemRectMin().x;
				float nextButtonPos = lastButtonPos + ImGui::GetStyle().ItemSpacing.x + buttonSize;

				if (i != 0 && nextButtonPos < windowRightBorder)
					ImGui::SameLine();

				ImGui::PushID(assignmentUser->GetId());
				if (ImGui::Button(assignmentUser->GetName()))
					removedUserId = assignmentUser->GetId();
				ImGui::PopID();
				 
				i++;
			}
			ImGui::PopStyleColor(2);

			// Remove user from assignment outside loop
			if (removedUserId != -1)
			{
				editingAssignmentData.RemoveUser(removedUserId);
				removedUserId = -1;
			}

			ImGui::Text("Deadline");
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

			if (ImGui::DatePicker("##Deadline", editingAssignmentData.DeadLineTm))
				editingAssignmentData.UpdateDeadLine();
			ImGui::PopStyleVar();

			switch (editingAssignmentData.Error)
			{
			case CreateAssignmentErrorType::None:
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 33.0f); // Skip height of the error text height
				break;
			case CreateAssignmentErrorType::WrongDate:
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Date has to be in the future!").x) / 2);
				ImGui::TextColored(errorColor, "Date has to be in the future!");
				break;
			case CreateAssignmentErrorType::NoUser:
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("No user added!").x) / 2);
				ImGui::TextColored(errorColor, "No user added!");
				break;
			case CreateAssignmentErrorType::MaxAttachmentSizeReached:
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Maximum size of file is 20MB!").x) / 2);
				ImGui::TextColored(errorColor, "Maximum size of file is 20MB!");
				break;
			}

			// Center buttons x position
			float buttonsWidth = ImGui::CalcTextSize("Cancel").x + ImGui::CalcTextSize("Create").x + ImGui::GetStyle().FramePadding.x * 2;
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonsWidth) / 2);

			if (ImGui::Button("Cancel"))
			{
				editingAssignmentData = AssignmentData();
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Create"))
			{
				if (editingAssignmentData.GetUserCount())
				{
					std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

					if (difftime(editingAssignmentData.DeadLine, now) > 0)
					{
						// Create assignment
						Core::Command command((uint32_t)MessageResponses::LinkAssignmentToUser);
						command.SetType(Core::CommandType::Command);

						command.SetCommandString("INSERT INTO assignments (team_id, name, description, deadline) VALUES (?, ?, ?, ?);");
						command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));
						command.AddData(new Core::DatabaseString(editingAssignmentData.Name));
						command.AddData(new Core::DatabaseString(editingAssignmentData.Description));
						command.AddData(new Core::DatabaseTimestamp(editingAssignmentData.DeadLine));
						SendCommandMessage(command);

						// Send notifications to users about new assignment
						for (auto& [id, assignmentUser] : editingAssignmentData.GetUsers())
						{
							Core::Command notificationCommand((uint32_t)MessageResponses::None);
							notificationCommand.SetType(Core::CommandType::Command);

							std::string message = "You have new assignment in ";
							message += loggedUser.GetSelectedTeam().GetName();

							notificationCommand.SetCommandString("INSERT INTO notifications (user_id, message) VALUES (?, ?);");
							notificationCommand.AddData(new Core::DatabaseInt(id));
							notificationCommand.AddData(new Core::DatabaseString(message));
							SendCommandMessage(notificationCommand);
						}

						ImGui::CloseCurrentPopup();
					}
					else
						editingAssignmentData.Error = CreateAssignmentErrorType::WrongDate;
				}
				else
					editingAssignmentData.Error = CreateAssignmentErrorType::NoUser;
			}

			ImGui::EndPopup();
		}
	}

	void ClientApp::RenderEditAssignmentPopup(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Center assignment creation popup if it's open
		if (ImGui::IsPopupOpen("AssignmentEdit"))
		{
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(400.0f, 0.0f));
		}

		// Render assignment creation popup
		if (ImGui::BeginPopupModal("AssignmentEdit", nullptr, flags))
		{
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Edit assignment").x) / 2);
			ImGui::Text("Edit assignment");

			ImGui::Text("Name");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputText("##Name", editingAssignmentData.Name, editingAssignmentData.GetNameSize());

			ImGui::Text("Task");
			ImGui::InputTextMultiline("##TaskInput", editingAssignmentData.Description, editingAssignmentData.GetDescriptionSize(), ImVec2(ImGui::GetContentRegionAvail().x, 100.0f));

			ImGui::Text("Deadline");
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

			if (ImGui::DatePicker("##Deadline", editingAssignmentData.DeadLineTm))
				editingAssignmentData.UpdateDeadLine();
			ImGui::PopStyleVar();

			switch (editingAssignmentData.Error)
			{
			case Client::CreateAssignmentErrorType::None:
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 33.0f); // Skip height of the error text height
				break;
			case Client::CreateAssignmentErrorType::WrongDate:
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Date has to be in the future!").x) / 2);
				ImGui::TextColored(errorColor, "Date has to be in the future!");
				break;
			}

			// Center buttons x position
			float buttonsWidth = ImGui::CalcTextSize("Cancel").x + ImGui::CalcTextSize("Create").x + ImGui::GetStyle().FramePadding.x * 2;
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonsWidth) / 2);

			if (ImGui::Button("Cancel"))
			{
				editingAssignmentData = AssignmentData();

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Edit"))
			{
				Ref<Assignment> assignment = loggedUser.GetAssignments()[editingAssignmentData.AssignmentId];

				if (strcmp(editingAssignmentData.Name, assignment->GetName()) || strcmp(editingAssignmentData.Description, assignment->GetDescription()) || difftime(editingAssignmentData.DeadLine, assignment->GetDeadLine()))
				{
					std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

					if (difftime(editingAssignmentData.DeadLine, now) > 0)
					{
						Core::Command command((uint32_t)MessageResponses::None);
						command.SetType(Core::CommandType::Update);

						command.SetCommandString("UPDATE assignments set name = ?, description = ?, deadline = ? WHERE id = ?;");
						command.AddData(new Core::DatabaseString(editingAssignmentData.Name));
						command.AddData(new Core::DatabaseString(editingAssignmentData.Description));
						command.AddData(new Core::DatabaseTimestamp(editingAssignmentData.DeadLine));
						command.AddData(new Core::DatabaseInt(editingAssignmentData.AssignmentId));

						SendCommandMessage(command);

						for (auto& [id, assignmentUser] : assignment->GetUsers())
						{
							std::string message = "Your assignment ";
							message += assignment->GetName();
							message += " in ";
							message += loggedUser.GetSelectedTeam().GetName();
							message += " has been updated";
							SendNotificationMessage(id, message.c_str());
						}

						editingAssignmentData = AssignmentData();
						ImGui::CloseCurrentPopup();
					}
					else
						editingAssignmentData.Error = CreateAssignmentErrorType::WrongDate;
				}
				else
					ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ClientApp::RenderDeleteAssignmentPopup(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Center delete assignment popup if it's open
		if (ImGui::IsPopupOpen("AssignmentDelete"))
		{
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f));
		}

		// Render delete assignment popup
		if (ImGui::BeginPopupModal("AssignmentDelete", nullptr, flags))
		{
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("You want to delete this assignment?").x) / 2);
			ImGui::Text("You want to delete this assignment?");

			// Center buttons x position
			float buttonsWidth = ImGui::CalcTextSize("No").x + ImGui::CalcTextSize("Yes").x + ImGui::GetStyle().FramePadding.x * 2;
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonsWidth) / 2);

			if (ImGui::Button("No"))
				ImGui::CloseCurrentPopup();

			ImGui::SameLine();
			if (ImGui::Button("Yes"))
			{
				DeleteAssignment(editingAssignmentData.AssignmentId);
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ClientApp::RenderRateAssignmentPopup(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Center assignment rate popup if it's open
		if (ImGui::IsPopupOpen("AssignmentRate"))
		{
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(400.0f, 0.0f));
		}

		// Render assignment rate popup
		if (ImGui::BeginPopupModal("AssignmentRate", nullptr, flags))
		{
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Rate assignment").x) / 2);
			ImGui::Text("Rate assignment");

			ImGui::Text("Description");
			ImGui::InputTextMultiline("##Description", editingAssignmentData.Description, editingAssignmentData.GetDescriptionSize(), ImVec2(ImGui::GetContentRegionAvail().x, 100.0f));

			ImGui::Text("Rating");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::SliderInt("##RatingSlider", &editingAssignmentData.Rating, 0, 10);

			// Center buttons x position
			float buttonsWidth = ImGui::CalcTextSize("Cancel").x + ImGui::CalcTextSize("Rate").x + ImGui::GetStyle().FramePadding.x * 2;
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonsWidth) / 2);

			if (ImGui::Button("Cancel"))
			{
				editingAssignmentData = AssignmentData();

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Rate"))
			{
				Core::Command command((uint32_t)MessageResponses::None);
				command.SetType(Core::CommandType::Update);

				command.SetCommandString("UPDATE assignments set status = 'rated', rating = ?, rating_description = ? WHERE id = ?;");
				command.AddData(new Core::DatabaseInt(editingAssignmentData.Rating));
				command.AddData(new Core::DatabaseString(editingAssignmentData.Description));
				command.AddData(new Core::DatabaseInt(editingAssignmentData.AssignmentId));

				SendCommandMessage(command);

				for (auto& [id, assignmentUser] : editingAssignmentData.GetUsers())
				{
					std::string message = "Your assignment ";
					message += editingAssignmentData.Name;
					message += " in ";
					message += loggedUser.GetSelectedTeam().GetName();
					message += " has been rated";
					SendNotificationMessage(id, message.c_str());
				}

				editingAssignmentData = AssignmentData();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ClientApp::RenderCommonAssignmentProperties(Ref<Assignment> assignment)
	{
		ImGui::BeginDisabled();
		ImGui::Text("Task");
		ImGui::InputTextMultiline("##Task", (char*)assignment->GetDescription(), assignment->GetDescriptionSize(), ImVec2(ImGui::GetContentRegionAvail().x, 100.0f));

		ImGui::Text("Assigned users");
		if (ImGui::BeginListBox("##UserListbox", ImVec2(ImGui::GetContentRegionAvail().x, 65.0f)))
		{
			for (auto& [id, assignmentUser] : assignment->GetUsers())
				ImGui::Text(assignmentUser->GetName());

			ImGui::EndListBox();
		}
		ImGui::EndDisabled();

		if (assignment->GetOwnerAttachmentCount())
		{
			ImGui::Text("Attachments");

			if (ImGui::BeginTable("OwnerAttachmentTable", 2))
			{
				ImGui::TableSetupColumn("AttachmentName", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("DownloadButton", ImGuiTableColumnFlags_WidthFixed, 100.0f);

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
				for (Ref<File> attachment : assignment->GetAttachments())
				{
					if (attachment->IsByUser())
						continue;

					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text(attachment->GetName());

					int attachmentId = attachment->GetId();
					ImGui::TableNextColumn();
					ImGui::PushID(attachmentId);
					if (ImGui::Button("Download"))
					{
						SetFileExtension(attachment->GetName());
						DownloadAttachment(attachmentId);
					}
					ImGui::PopID();
				}
				ImGui::PopStyleVar();

				ImGui::EndTable();
			}
		}
	}

	void ClientApp::RenderUserPage(ImGuiWindowFlags flags)
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::SetNextWindowPos(ImVec2(leftSidePanelWidth, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - leftSidePanelWidth - rightSidePanelWidth, io.DisplaySize.y));
		ImGui::Begin("UserPageNotifications", nullptr, flags);

		ImGui::BeginTabBar("UserPageTabBar");
		if (ImGui::BeginTabItem("Notifications"))
		{
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 110.0f);
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
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120.0f);
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

		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - rightSidePanelWidth, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(rightSidePanelWidth, io.DisplaySize.y));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		ImGui::Begin("UserPageSettings", nullptr, flags);

		ImGui::PopStyleVar();
		ImGui::Text("Change name");
		static char firstNameBuffer[CHAR_BUFFER_SIZE] = {};
		static char lastNameBuffer[CHAR_BUFFER_SIZE] = {};

		ImGui::InputTextWithHint("##ChangeFirstName", "First name", firstNameBuffer, sizeof(firstNameBuffer));
		ImGui::InputTextWithHint("##ChangeLastName", "Last name", lastNameBuffer, sizeof(lastNameBuffer));
		if (ImGui::Button("Change##name"))
		{
			if (strlen(firstNameBuffer) && strlen(lastNameBuffer))
			{
				Core::Command command((uint32_t)MessageResponses::ChangeUsername);
				command.SetType(Core::CommandType::Update);

				command.SetCommandString("UPDATE users set first_name = ?, last_name = ? WHERE id = ?;");
				command.AddData(new Core::DatabaseString(firstNameBuffer));
				command.AddData(new Core::DatabaseString(lastNameBuffer));
				command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

				SendCommandMessage(command);

				memset(firstNameBuffer, 0, sizeof(firstNameBuffer));
				memset(lastNameBuffer, 0, sizeof(lastNameBuffer));
			}
			else
				changeUsernameState = ChangeUsernameState::NotFilled;
		}

		switch (changeUsernameState)
		{
		case ChangeUsernameState::None:
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 33.0f); // Skip height of the error text height
			break;
		case ChangeUsernameState::Error:
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Something went wrong!").x) / 2);
			ImGui::TextColored(errorColor, "Something went wrong!");
			break;
		case ChangeUsernameState::NotFilled:
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("All inputs must be filled!").x) / 2);
			ImGui::TextColored(errorColor, "All inputs must be filled!");
			break;
		case ChangeUsernameState::ChangeSuccessful:
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Name has been changed successfully!").x) / 2);
			ImGui::TextColored(successColor, "Name has been changed successfully!");
			break;
		}

		ImGui::Text("Change password");
		static char passwordBuffer[CHAR_BUFFER_SIZE] = {};
		static char checkPasswordBuffer[CHAR_BUFFER_SIZE] = {};

		ImGui::InputTextWithHint("##ChangePassword", "New password", passwordBuffer, sizeof(passwordBuffer), ImGuiInputTextFlags_Password);
		ImGui::InputTextWithHint("##ChangeCheckPassword", "Check password", checkPasswordBuffer, sizeof(checkPasswordBuffer), ImGuiInputTextFlags_Password);
		if (ImGui::Button("Change##password"))
		{
			if (strlen(passwordBuffer) && strlen(checkPasswordBuffer))
			{
				if (!strcmp(passwordBuffer, checkPasswordBuffer))
				{
					std::string hash = Core::GenerateHash(passwordBuffer);

					Core::Command command((uint32_t)MessageResponses::ChangePassword);
					command.SetType(Core::CommandType::Update);

					command.SetCommandString("UPDATE users set password = ? WHERE id = ?;");
					command.AddData(new Core::DatabaseString(hash));
					command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

					SendCommandMessage(command);

					memset(passwordBuffer, 0, sizeof(passwordBuffer));
					memset(checkPasswordBuffer, 0, sizeof(checkPasswordBuffer));
				}
				else
					changePasswordState = ChangePasswordState::NotMatching;
			}
			else
				changePasswordState = ChangePasswordState::NotFilled;
		}

		switch (changePasswordState)
		{
		case ChangePasswordState::None:
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 33.0f); // Skip height of the error text height
			break;
		case ChangePasswordState::Error:
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Somethink went wrong!").x) / 2);
			ImGui::TextColored(errorColor, "Somethink went wrong!");
			break;
		case ChangePasswordState::NotFilled:
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("All inputs must be filled!").x) / 2);
			ImGui::TextColored(errorColor, "All inputs must be filled!");
			break;
		case ChangePasswordState::NotMatching:
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Passwords does not match!").x) / 2);
			ImGui::TextColored(errorColor, "Passwords does not match!");
			break;
		case ChangePasswordState::ChangeSuccessful:
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Password has been changed successfully!").x) / 2);
			ImGui::TextColored(successColor, "Password has been changed successfully!");
			break;
		}

		ImGui::End();
	}

	void ClientApp::ResetActionStates()
	{
		inviteState = InviteState::None;
		changeUsernameState = ChangeUsernameState::None;
		changePasswordState = ChangePasswordState::None;
	}

	void ClientApp::SetStyle()
	{
		// Get ImGui objects
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		
		// Allow specific characters
		ImVector<ImWchar> ranges;
		ImFontGlyphRangesBuilder builder;
		builder.AddText((const char*)u8"ášťřňčěžýíéóďúůäëïüöÁŠŤŘŇČĚŽÝÍÉÓĎŮÄËÏÖÜˇ´¨");
		builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
		builder.BuildRanges(&ranges);

		// Load fonts
		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;
		io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)&robotoMediumFontData, sizeof(robotoMediumFontData), 20.0f, &fontConfig, ranges.Data);
		fonts["Heading"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoMediumFontData, sizeof(robotoMediumFontData), 38.0f, &fontConfig, ranges.Data);
		fonts["Regular"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 16.0f, &fontConfig, ranges.Data);
		fonts["Regular22"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 22.0f, &fontConfig, ranges.Data);
		fonts["Regular16"] = io.Fonts->AddFontFromMemoryTTF((void*)&robotoThinFontData, sizeof(robotoThinFontData), 16.0f, &fontConfig, ranges.Data);
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
		style.Colors[ImGuiCol_FrameBgHovered] = style.Colors[ImGuiCol_FrameBg];
		style.Colors[ImGuiCol_FrameBgActive] = style.Colors[ImGuiCol_FrameBg];

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

		style.Colors[ImGuiCol_SliderGrab] = style.Colors[ImGuiCol_Button];
		style.Colors[ImGuiCol_SliderGrabActive] = style.Colors[ImGuiCol_ButtonActive];
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

	void ClientApp::SendAttachment(Ref<File> attachment)
	{
		Ref<Core::Message> message = CreateRef<Core::Message>();
		message->Header.Type = Core::MessageType::UploadFile;
		attachment->Serialize(message->Body.Content);
		message->Header.Size = message->Body.Content->GetSize();

		networkInterface->SendMessagePackets(message);
	}

	void ClientApp::DownloadAttachment(uint32_t attachmentId)
	{
		Ref<Core::Message> message = CreateRef<Core::Message>();
		message->Header.Type = Core::MessageType::DownloadFile;
		message->CreateBody<uint32_t>(attachmentId);

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
		
		command.SetCommandString("SELECT id, password, first_name, last_name, email, role FROM users WHERE email = ?;");
		command.AddData(new Core::DatabaseString(loginData.Email));

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
		command.AddData(new Core::DatabaseString(Core::GenerateHash(registerData.Password).c_str()));

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

	void ClientApp::UpdateLoggedUser()
	{
		Core::Command command((uint32_t)MessageResponses::UpdateLoggedUser);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT first_name, last_name, email, role FROM users WHERE id = ?;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

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

	// Send command to read all logged user's assignments
	void ClientApp::ReadUsersAssignments()
	{
		Core::Command command((uint32_t)MessageResponses::ProcessAssignments);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT assignments.id, assignments.name, assignments.description, assignments.status, assignments.rating, assignments.rating_description, assignments.deadline, assignments.submitted_at FROM users_assignments JOIN assignments ON users_assignments.assignment_id = assignments.id WHERE users_assignments.user_id = ? AND assignments.team_id = ? ORDER BY deadline LIMIT 200;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetId()));
		command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));

		SendCommandMessage(command);
	}

	// Send command to read all logged user's notifications
	void ClientApp::ReadUsersNotifications()
	{
		Core::Command command((uint32_t)MessageResponses::ProcessNotifications);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT id, message FROM notifications WHERE user_id = ? ORDER BY id DESC;");
		command.AddData(new Core::DatabaseInt(loggedUser.GetId()));

		SendCommandMessage(command);
	}

	// Send command to read all teams assignments (team owner view)
	void ClientApp::ReadSelectedTeamAssignments()
	{
		Core::Command command((uint32_t)MessageResponses::ProcessAssignments);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT id, name, description, status, rating, rating_description, deadline, submitted_at FROM assignments WHERE team_id = ? ORDER BY deadline LIMIT 200");
		command.AddData(new Core::DatabaseInt(loggedUser.GetSelectedTeam().GetId()));

		SendCommandMessage(command);
	}

	void ClientApp::ReadAssignmentsUsers(Ref<Assignment> assignment)
	{
		Core::Command command((uint32_t)MessageResponses::ProcessAssignmentsUsers);
		command.SetType(Core::CommandType::Query);

		command.SetCommandString("SELECT assignment_id, users.id, users.first_name, users.last_name FROM users_assignments JOIN users ON users_assignments.user_id = users.id WHERE users_assignments.assignment_id = ?;");
		command.AddData(new Core::DatabaseInt(assignment->GetId()));

		SendCommandMessage(command);
	}

	void ClientApp::ReadAssignmentsAttachments(Ref<Assignment> assignment)
	{
		Ref<Core::Message> message = CreateRef<Core::Message>();
		message->Header.Type = Core::MessageType::ReadFileName;
		message->CreateBody<uint32_t>(assignment->GetId());

		networkInterface->SendMessagePackets(message);
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

	// Read all team's assignments or user's assignments based on team ownership
	void ClientApp::ReadAssignments()
	{
		if (loggedUser.IsTeamOwner(loggedUser.GetSelectedTeam()))
			ReadSelectedTeamAssignments();
		else
			ReadUsersAssignments();
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

	void ClientApp::DeleteAssignment(uint32_t assignmentId)
	{
		Core::Command command((uint32_t)MessageResponses::None);
		command.SetType(Core::CommandType::Command);

		command.AddData(new Core::DatabaseInt(assignmentId));
		command.SetCommandString("DELETE FROM users_assignments WHERE assignment_id = ?;");
		SendCommandMessage(command);

		command.SetCommandString("DELETE FROM attachments WHERE assignment_id = ?;");
		SendCommandMessage(command);

		command.SetCommandString("DELETE FROM assignments WHERE id = ?;");
		SendCommandMessage(command);
	}

	void ClientApp::DeleteAttachment(uint32_t attachmentId)
	{
		Core::Command command((uint32_t)MessageResponses::None);
		command.SetType(Core::CommandType::Command);

		command.SetCommandString("DELETE FROM attachments WHERE id = ?;");
		command.AddData(new Core::DatabaseInt(attachmentId));
		SendCommandMessage(command);
	}

	// Send command to delete selected team
	void ClientApp::DeleteSelectedTeam()
	{
		Core::Command command((uint32_t)MessageResponses::None);
		command.SetType(Core::CommandType::Command);

		for (auto& [id, assignment] : loggedUser.GetAssignments())
			DeleteAssignment(id);

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