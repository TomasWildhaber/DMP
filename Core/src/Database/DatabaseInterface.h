#pragma once
#include "Utils/Memory.h"
#include "Response.h"

namespace Core
{
	class DatabaseInterface
	{
	public:
		virtual ~DatabaseInterface() = default;

		virtual inline const bool IsConnected() const = 0;

		virtual bool Execute(const char* query) = 0;
		virtual bool Query(const char* query) = 0;
		virtual void FetchData(Response& response) = 0;

		static Ref<DatabaseInterface> Create(const char* database, const char* username, const char* password);
	};
}