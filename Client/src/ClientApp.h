#pragma once
#include "Core/Application.h"
#include "Networking/NetworkClientInterface.h"
#include "Networking/MessageQueue.h"
#include "User.h"
#include "Database/Command.h"

struct ImFont;

namespace Client
{
	enum class LoginErrorType
	{
		None = 0,
		NotFilled,
		Incorrect,
	};

	enum class RegisterErrorType
	{
		None = 0,
		NotFilled,
		NotMatching,
		Existing,
	};

	enum class ClientState
	{
		None = 0,
		Login,
		Register,
		Home
	};

	struct LoginData
	{
		std::string Email;
		std::string PasswordHash;
		LoginErrorType Error = LoginErrorType::None;
	};

	struct RegisterData
	{
		std::string FirstName;
		std::string LastName;
		std::string Email;
		std::string PasswordHash;
		RegisterErrorType Error = RegisterErrorType::None;
	};

	class ClientApp : public Core::Application
	{
		using FontMap = std::unordered_map<const char*, ImFont*>;
	public:
		ClientApp(const Core::ApplicationSpecifications& specs);

		virtual void OnEvent(Core::Event& e) override;
		virtual void ProcessMessageQueue() override;
		void ProcessMessage();

		void Render();
	private:
		void SetStyle();

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

		Ref<Core::NetworkClientInterface> networkInterface;
		Core::MessageQueue messageQueue;

		FontMap fonts;
		ClientState state = ClientState::None;

		LoginData loginData;
		RegisterData registerData;
		User user;
	};
}