#pragma once
#include "Core/Application.h"
#include "Networking/NetworkClientInterface.h"
#include "Networking/MessageQueue.h"

struct ImFont;

namespace Client
{
	enum class ClientState
	{
		None = 0,
		Login,
		Register,
		Home
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

		Ref<Core::NetworkClientInterface> networkInterface;
		Core::MessageQueue messageQueue;

		FontMap fonts;
		ClientState state = ClientState::None;
	};
}