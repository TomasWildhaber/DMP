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
		std::string PasswordHash;
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
		// Map fonts to names for easy access
		using FontMap = std::unordered_map<const char*, ImFont*>;
	public:
		ClientApp(const Core::ApplicationSpecifications& specs);

		virtual void OnEvent(Core::Event& e) override;
		virtual void ProcessMessageQueue() override;
		void ProcessMessage();
	private:
		// UI functions
		void Render();
		void SetStyle();

		// Event functions
		void OnConnect(Core::ConnectedEvent& e);
		void OnDisconnect(Core::DisconnectedEvent& e);
		void OnMessageSent(Core::MessageSentEvent& e);
		void OnMessageAccepted(Core::MessageAcceptedEvent& e);

		// Sending methods
		void SendLoginMessage();
		void SendRegisterMessage();
		void SendCommandMessage(Core::Command& command);

		// Reading methods
		void ReadUserTeams();
		void ReadTeamMessages();
		void ReadTeamUsers();

		// Modifying methods
		void DeleteTeam(Team& team);
		void RenameTeam(Team& team, const char* teamName);

		void ValidateUserSelection(Ref<Team> firstTeam);

		Ref<Core::NetworkClientInterface> networkInterface;
		Core::MessageQueue messageQueue;

		FontMap fonts;
		ClientState state = ClientState::None;

		LoginData loginData;
		RegisterData registerData;
		User user;
	};
}