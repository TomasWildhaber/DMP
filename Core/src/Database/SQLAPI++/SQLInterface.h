#pragma once
#include "Database/DatabaseInterface.h"
#include <jdbc/cppconn/driver.h>
#include "jdbc/mysql_connection.h"
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/prepared_statement.h>

namespace Core
{
	class SQLInterface : public DatabaseInterface
	{
	public:
		SQLInterface(const char* address, const char* database, const char* username, const char* password);

		virtual inline const bool IsConnected() const override { return connection->isValid(); };

		virtual bool Execute(Command& command) override;
		virtual bool Query(Command& command) override;
		virtual bool Update(Command& command) override;
		virtual void FetchData(Response& response) override;
	private:
		void loadValues(Command& command);

		sql::Driver* driver;
		Ref<sql::Connection> connection;
		Ref<sql::PreparedStatement> statement;
		Ref<sql::ResultSet> result;
	};
}