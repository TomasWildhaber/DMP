#pragma once
#include "Core/Application.h"

namespace Server
{
	class ServerApp : public Core::Application
	{
	public:
		ServerApp(const Core::ApplicationSpecifications& specs);
	};
}