#pragma once
#include "Core/Application.h"
#include "Networking/NetworkClientInterface.h"
#include "Networking/MessageQueue.h"
#include "User.h"
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
		ProcessTeams,
		ProcessTeamMessages,
		ProcessTeamUsers,
	};

	// Enum for rendering client states - rendering different windows based on state
	enum class ClientState
	{
		None = 0,
		Login,
		Register,
		Home
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
		virtual void ProcessMessageQueue() override;
		void ProcessMessage();
	private:
		// UI functions
		void SetStyle();
		void Render();

		void RenderLoginWindow(ImGuiWindowFlags flags);
		void RenderRegisterWindow(ImGuiWindowFlags flags);
		void RenderHomeWindow(ImGuiWindowFlags flags);

		// Event functions
		void OnConnect(Core::ConnectedEvent& e);
		void OnDisconnect(Core::DisconnectedEvent& e);
		void OnMessageSent(Core::MessageSentEvent& e);
		void OnMessageAccepted(Core::MessageAcceptedEvent& e);

		// Sending methods
		void SendCommandMessage(Core::Command& command);
		void SendLoginMessage();
		void SendRegisterMessage();

		// Reading methods
		void ReadUsersTeams();
		void ReadSelectedTeamMessages();
		void ReadSelectedTeamUsers();

		// Modifying methods
		void DeleteSelectedTeam();
		void RenameSelectedTeam(const char* teamName);

		// Networking
		Ref<Core::NetworkClientInterface> networkInterface;
		Core::MessageQueue messageQueue;

		// Map for saving fonts
		FontMap fonts;

		// Enum for rendering ui windows by the state of application
		ClientState state = ClientState::None;

		User loggedUser;

		// For saving data from login and register forms
		LoginData loginData;
		RegisterData registerData;
	};
}