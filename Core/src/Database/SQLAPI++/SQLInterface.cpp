#include "pch.h"
#include "SQLInterface.h"
#include "Debugging/Log.h"

#include <jdbc/cppconn/exception.h>

namespace Core
{
	Ref<DatabaseInterface> DatabaseInterface::Create(const char* database, const char* username, const char* password)
	{
		return new SQLInterface(database, username, password);
	}

	SQLInterface::SQLInterface(const char* database, const char* username, const char* password)
	{
		try
		{
			driver = get_driver_instance();
			connection = driver->connect("tcp://127.0.0.1:3306", "dmp", "Tester_123");
			connection->setSchema("dmp");
			statement = connection->createStatement();
			INFO("Database connected!");
		}
		catch (const sql::SQLException& e)
		{
			ERROR("Database connection failed!");
			ERROR("{0}", e.getSQLStateCStr());
		}
	}

	bool SQLInterface::Execute(const char* query)
	{
		try
		{
			statement->execute(query);
			return true;
		}
		catch (const sql::SQLException& e)
		{
			ERROR("SQL statement error: {0}", e.getSQLStateCStr());
			return false;
		}
	}

	bool SQLInterface::Query(const char* query)
	{
		try
		{
			result = statement->executeQuery(query);
			return true;
		}
		catch (const sql::SQLException& e)
		{
			ERROR("SQL query error: {0}", e.getSQLStateCStr());
			return false;
		}
	}

	void SQLInterface::FetchData(Response& response)
	{
		auto metadata = result->getMetaData();

		uint32_t i = 1;
		while (result->next())
		{
			for (uint32_t i = 0; i < metadata->getColumnCount(); i++)
			{
				// auto typeName = metadata->getColumnTypeName(i + 1);
				int type = metadata->getColumnType(i + 1);

				switch (type)
				{
				case 5:
					response.AddData(new DatabaseInt(result->getInt(i + 1)));
					break;
				case 13:
					response.AddData(new DatabaseString(result->getString(i + 1).c_str()));
					break;
				}
			}

			i++;
		}
	}
}