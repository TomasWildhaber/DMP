#pragma once
#include "DatabaseData.h"

namespace Core
{
	enum class CommandType
	{
		None = 0,
		Query,
		Command,
	};

	class Command
	{
	public:
		Command(uint32_t id) : taskId(id) {}

		inline const uint32_t GetTaskId() const { return taskId; }
		inline const CommandType GetType() const { return type; }
		inline const char* GetCommandString() const { return commandString; }

		inline void SetCommandString(const char* command)
		{
			strncpy(commandString, command, sizeof(commandString) - 1);
			commandString[sizeof(commandString) - 1] = '\0';
		}

		inline void SetType(CommandType Type) { type = Type; }
	private:
		char commandString[256] = {};
		CommandType type = CommandType::None;
		uint32_t taskId = 0;
	};
}