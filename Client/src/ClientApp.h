#pragma once
#include "Core/Application.h"
#include "Networking/NetworkClientInterface.h"
#include "Networking/MessageQueue.h"
#include "Client/ErrorTypes.h"
#include "Client/User.h"
#include "Database/Command.h"
#include "Client/Assignment.h"

#define CHAR_BUFFER_SIZE 256

struct ImFont;

namespace Client
{
	// Enum of server response ids (instead of using raw numbers, casting them to uint32 anyway)
	enum class MessageResponses : uint32_t
	{
		None = 0, // No response
		Login,
		Register,
		CheckEmail,
		LinkTeamToUser,
		UpdateTeams, // Reserved by server
		UpdateMessages, // Reserved by server
		UpdateUsers, // Reserved by server
		UpdateInvites, // Reserved by server
		UpdateNotifications, // Reserved by server
		UpdateAssignments, // Reserved by server
		LinkAssignmentToUser,
		UpdateLoggedUser,
		CheckInvite,
		CheckTeam,
		ChangeUsername,
		ChangePassword,
		ProcessTeams,
		ProcessTeamMessages,
		ProcessTeamUsers,
		ProcessInvites,
		ProcessNotifications,
		ProcessAssignments,
		ProcessAssignmentsUsers,
	};

	// Enum for rendering client states - rendering different windows based on state
	enum class ClientState
	{
		None = 0,
		Login,
		Register,
		Home,
	};

	// Struct for saving data from login form
	struct LoginData
	{
		char Email[CHAR_BUFFER_SIZE] = {};
		char Password[CHAR_BUFFER_SIZE] = {};
		LoginErrorType Error = LoginErrorType::None;
	};

	// Struct for saving data from register form
	struct RegisterData
	{
		char FirstName[CHAR_BUFFER_SIZE] = {};
		char LastName[CHAR_BUFFER_SIZE] = {};
		char Email[CHAR_BUFFER_SIZE] = {};
		char Password[CHAR_BUFFER_SIZE] = {};
		char CheckPassword[CHAR_BUFFER_SIZE] = {};
		RegisterErrorType Error = RegisterErrorType::None;
	};

	// Class for client inherited from Core::Application class - return instance of it to Core::CreateApplication
	class ClientApp : public Core::Application
	{
		// Alias to map fonts to names for easy access
		using FontMap = std::unordered_map<const char*, ImFont*>;
	public:
		ClientApp(const Core::ApplicationSpecifications& specs);

		virtual void OnEvent(Core::Event& e) override;
	private:
		// Config file methods
		void ReadConfigFile() override;
		void WriteConfigFile() override;

		// Event functions
		void OnConnect(Core::ConnectedEvent& e);
		void OnDisconnect(Core::DisconnectedEvent& e);
		void OnMessageSent(Core::MessageSentEvent& e);
		void OnMessageAccepted(Core::MessageAcceptedEvent& e);

		// Networking methods
		virtual void ProcessMessageQueue() override;
		void ProcessMessage();

		// UI functions
		void SetStyle();
		void Render();

		// Render windows
		void RenderLoginWindow(ImGuiWindowFlags flags);
		void RenderRegisterWindow(ImGuiWindowFlags flags);
		void RenderHomeWindow(ImGuiWindowFlags flags);

		// Render popup windows
		void RenderCreateTeamPopup(ImGuiWindowFlags flags);
		void RenderRenameTeamPopup(ImGuiWindowFlags flags);
		void RenderDeleteTeamPopup(ImGuiWindowFlags flags);

		void RenderCreateAssignmentPopup(ImGuiWindowFlags flags);
		void RenderEditAssignmentPopup(ImGuiWindowFlags flags);
		void RenderDeleteAssignmentPopup(ImGuiWindowFlags flags);

		void RenderUserPage(ImGuiWindowFlags flags);

		void ResetActionStates();

		// Sending methods
		void SendCommandMessage(Core::Command& command);
		void SendLoginMessage();
		void SendRegisterMessage();
		void SendNotificationMessage(uint32_t userId, const char* message);
		// Checking if something exists in database
		void SendCheckEmailMessage(const char* email); // Check if email is already registered
		void SendCheckInviteMessage(uint32_t userId); // Check if user already has invite
		void SendCheckTeamMessage(uint32_t userId); // Check if user is already in the team

		// Reading methods
		void UpdateLoggedUser();
		void ReadUsersTeams();
		void ReadUsersInvites();
		void ReadUsersNotifications();

		void ReadUsersAssignments();
		void ReadSelectedTeamAssignments();
		void ReadAssignmentsUsers(Ref<Assignment> assignment);

		void ReadSelectedTeamMessages();
		void ReadSelectedTeamUsers();

		void ReadAssignments();

		// Modifying methods
		void DeleteInvite(uint32_t inviteId);
		void DeleteAllInvites();
		void DeleteAllNotifications();

		void DeleteAssignment(uint32_t assignmentId);

		void DeleteSelectedTeam();
		void RenameSelectedTeam(const char* teamName);

		// Networking
		Ref<Core::NetworkClientInterface> networkInterface;
		Core::MessageQueue messageQueue;

		// Networking target specifications
		std::string address;
		uint32_t port = 0;

		// Map for saving fonts
		FontMap fonts;

		// Enum for rendering ui windows by the state of application
		ClientState state = ClientState::None;

		// Action states
		CreateAssignmentErrorType createAssignmentError = CreateAssignmentErrorType::None; // Error for creating assignment
		InviteState inviteState = InviteState::None; // State for inviting users into team
		ChangeUsernameState changeUsernameState = ChangeUsernameState::None; // State for changing username
		ChangePasswordState changePasswordState = ChangePasswordState::None; // State for changing password

		User loggedUser;

		// For saving data from login and register forms
		LoginData loginData;
		RegisterData registerData;

		float leftSidePanelWidth = 300.0f;
		float rightSidePanelWidth = 400.0f;
	};
}