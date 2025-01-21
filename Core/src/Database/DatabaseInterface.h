#pragma once
#include "Utils/Memory.h"
#include "Response.h"
#include "Command.h"

namespace Core
{
	class DatabaseInterface
	{
	public:
		virtual ~DatabaseInterface() = default;

		virtual inline const bool IsConnected() const = 0;

		virtual bool Execute(Command& command) = 0;
		virtual bool Query(Command& command) = 0;
		virtual bool Update(Command& command) = 0;
		virtual void FetchData(Response& response) = 0;

		static Ref<DatabaseInterface> Create(const char* address, const char* database, const char* username, const char* password);
	};
}