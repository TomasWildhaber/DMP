#pragma once
#include "Core/Application.h"

namespace Client
{
	class ClientApp : public Core::Application
	{
	public:
		ClientApp(const Core::ApplicationSpecifications& specs);
	};
}