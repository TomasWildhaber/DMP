#pragma once
#include "CommandBase.h"

namespace Core
{
	enum class CommandType
	{
		None = 0,
		Query,
		Command,
		Update,
	};

	class Command : public CommandBase
	{
	public:
		Command() = default;
		Command(uint32_t id) : CommandBase(id) {}

		inline const char* GetCommandString() const { return commandString; }
		inline const CommandType GetType() const { return type; }

		inline void SetType(CommandType Type) { type = Type; }

		inline void SetCommandString(const char* command)
		{
			strncpy(commandString, command, sizeof(commandString) - 1);
			commandString[sizeof(commandString) - 1] = '\0';
		}

		void Serialize(Ref<Buffer>& buffer) const override
		{
			std::ostringstream os;

			os.write(reinterpret_cast<const char*>(&taskId), sizeof(taskId));
			os.write(reinterpret_cast<const char*>(&type), sizeof(type));
			os.write(commandString, sizeof(commandString));

			uint32_t count = data.size();
			os.write(reinterpret_cast<const char*>(&count), sizeof(count));

			for (const auto& item : data)
			{
				DatabaseDataType type = item->GetType();
				os.write(reinterpret_cast<const char*>(&type), sizeof(type));
				item->Serialize(os);
			}

			buffer = CreateRef<Buffer>(os.str().size());
			buffer->Write(os.str().data(), os.str().size());
		}

		void Deserialize(Ref<Buffer>& buffer) override
		{
			std::istringstream is(std::string(buffer->GetData(), buffer->GetSize()));

			is.read(reinterpret_cast<char*>(&taskId), sizeof(taskId));
			is.read(reinterpret_cast<char*>(&type), sizeof(type));
			is.read(commandString, sizeof(commandString));

			uint32_t count = 0;
			is.read(reinterpret_cast<char*>(&count), sizeof(count));

			data.clear();
			data.reserve(count);

			for (uint32_t i = 0; i < count; ++i)
			{
				DatabaseDataType type = DatabaseDataType::None;
				is.read(reinterpret_cast<char*>(&type), sizeof(type));

				Ref<DatabaseData> item;
				switch (type)
				{
				case DatabaseDataType::Int:
					item = new DatabaseInt();
					break;
				case DatabaseDataType::String:
					item = new DatabaseString();
					break;
				case DatabaseDataType::Bool:
					item = new DatabaseBool();
					break;
				case DatabaseDataType::Timestamp:
					item = new DatabaseTimestamp();
					break;
				}

				item->Deserialize(is);
				data.push_back(item);
			}
		}
	private:
		char commandString[350] = {};
		CommandType type = CommandType::None;
	};
}