#pragma once
#include "CommandBase.h"

namespace Core
{
	class Response : public CommandBase
	{
		friend class SQLInterface;
	public:
		Response() = default;
		Response(uint32_t id) : CommandBase(id) {}

		void Serialize(Ref<Buffer>& buffer) const override
		{
			std::ostringstream os;

			os.write(reinterpret_cast<const char*>(&taskId), sizeof(taskId));

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
					// item = new DatabaseTimestamp();
					break;
				}

				item->Deserialize(is);
				data.push_back(item);
			}
		}
	};
}