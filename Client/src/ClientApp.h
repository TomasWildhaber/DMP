#pragma once
#include "Core/Application.h"
#include "Networking/NetworkClientInterface.h"
#include "Networking/MessageQueue.h"
#include "Client/User.h"
#include "Database/Command.h"

struct ImFont;

namespace Client
{
	// Enum of login error types
	enum class LoginErrorType
	{
		None = 0,
		NotFilled, // Not filled inputs
		Incorrect, // Incorrect input
	};

	// Enum of register error types
	enum class RegisterErrorType
	{
		None = 0,
		NotFilled, /// Not filled inputs
		NotMatching, // Not matching passwords
		WrongFormat, // Email is in wrong format
		Existing, // Existing email
	};

	enum class InviteState
	{
		None = 0,
		NotExisting, // User does not exist
		CannotInviteYourself, // Cannot invite yourself into you team
		AlreadyInvited, // User has already been invited
		AlreadyInTeam, // User is already in the team
		InviteSuccessful, // Invite has been successfully sent
	};

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
		CheckInvite,
		CheckTeam,
		ProcessTeams,
		ProcessTeamMessages,
		ProcessTeamUsers,
		ProcessInvites,
		ProcessNotifications,
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
		std::string Email;
		std::string Password;
		LoginErrorType Error = LoginErrorType::None;
	};

	// Struct for saving data from register form
	struct RegisterData
	{
		std::string FirstName;
		std::string LastName;
		std::string Email;
		std::string PasswordHash;
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

		void RenderLoginWindow(ImGuiWindowFlags flags);
		void RenderRegisterWindow(ImGuiWindowFlags flags);
		void RenderHomeWindow(ImGuiWindowFlags flags);

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
		void ReadUsersTeams();
		void ReadUsersInvites();
		void ReadUsersNotifications();
		void ReadSelectedTeamMessages();
		void ReadSelectedTeamUsers();

		// Modifying methods
		void DeleteInvite(uint32_t inviteId);
		void DeleteAllInvites();
		void DeleteAllNotifications();

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
		// State for inviting users into team
		InviteState inviteState = InviteState::None;

		User loggedUser;

		// For saving data from login and register forms
		LoginData loginData;
		RegisterData registerData;
	};
}