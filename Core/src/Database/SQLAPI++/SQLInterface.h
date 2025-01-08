#pragma once
#include "Database/DatabaseInterface.h"
#include <jdbc/cppconn/driver.h>
#include "jdbc/mysql_connection.h"
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>

namespace Core
{
	class SQLInterface : public DatabaseInterface
	{
	public:
		SQLInterface(const char* database, const char* username, const char* password);

		virtual inline const bool IsConnected() const override { return connection->isValid(); };

		virtual bool Execute(const char* query) override;
		virtual bool Query(const char* query) override;
		virtual void FetchData(Response& response) override;
	private:
		sql::Driver* driver;
		Ref<sql::Connection> connection;
		Ref<sql::Statement> statement;
		Ref<sql::ResultSet> result;
	};
}