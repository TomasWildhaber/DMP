#include "pch.h"
#include "SQLInterface.h"
#include "Debugging/Log.h"
#include "Core/Application.h"

#include <jdbc/cppconn/exception.h>

namespace Core
{
	Ref<DatabaseInterface> DatabaseInterface::Create(const char* address, const char* databaseName, const char* username, const char* password)
	{
		return new SQLInterface(address, databaseName, username, password);
	}

	SQLInterface::SQLInterface(const char* address, const char* databaseName, const char* username, const char* password) : database(databaseName)
	{
		try
		{
			driver = get_driver_instance();
			connection = driver->connect(address, username, password);
			connection->setSchema(database);
			connection->setClientOption("CHARSET", "utf8mb4");

			INFO("Database connected!");
		}
		catch (const sql::SQLException& e)
		{
			ERROR("Database connection failed, attempting reconnect!");
			ERROR("{0}", e.getSQLStateCStr());

			Application::Get().Restart();
		}
	}

	void SQLInterface::Reconnect()
	{
		ERROR("Database connection failed, attempting reconnect!");

		if (connection->reconnect())
		{
			connection->setSchema(database);
			connection->setClientOption("CHARSET", "utf8mb4");

			INFO("Database connected!");
		}
	}

	bool SQLInterface::Execute(Command& command)
	{
		try
		{
			statement = connection->prepareStatement(command.GetCommandString());
			loadValues(command);
			statement->execute();
			return true;
		}
		catch (const sql::SQLException& e)
		{
			ERROR("SQL statement error: {0}", e.getSQLStateCStr());
			return false;
		}
	}

	bool SQLInterface::Query(Command& command)
	{
		try
		{
			statement = connection->prepareStatement(command.GetCommandString());
			loadValues(command);
			result = statement->executeQuery();
			return true;
		}
		catch (const sql::SQLException& e)
		{
			ERROR("SQL query error: {0}", e.getSQLStateCStr());
			return false;
		}
	}

	bool SQLInterface::Update(Command& command)
	{
		try
		{
			statement = connection->prepareStatement(command.GetCommandString());
			loadValues(command);
			statement->executeUpdate();
			return true;
		}
		catch (const sql::SQLException& e)
		{
			ERROR("SQL update error: {0}", e.getSQLStateCStr());
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
				case 6: // int
					response.AddData(new DatabaseInt(result->getInt(i + 1)));
					break;
				case 13: // string
				case 15: // text
				case 22: // enum
					response.AddData(new DatabaseString(result->getString(i + 1).c_str()));
					break;
				default:
					ERROR("Unknown SQL data type {0}!", type);
					break;
				}
			}

			i++;
		}
	}

	void SQLInterface::loadValues(Command& command)
	{
		for (uint32_t i = 0; i < command.GetDataCount(); i++)
		{
			switch (command[i].GetType())
			{
			case DatabaseDataType::Int:
				statement->setInt(i + 1, *(int*)command[i].GetValue());
				break;
			case DatabaseDataType::String:
				statement->setString(i + 1, (const char*)command[i].GetValue());
				break;
			case DatabaseDataType::Bool:
				statement->setBoolean(i + 1, *(bool*)command[i].GetValue());
				break;
			case DatabaseDataType::Timestamp:
				statement->setDateTime(i + 1, (const char*)command[i].GetValue());
				break;
			}
		}
	}
}