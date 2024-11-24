#pragma once
#include "Core/Application.h"

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

		void Render();

		void SetStyle();
	private:
		FontMap fonts;
		ClientState state = ClientState::None;
	};
}