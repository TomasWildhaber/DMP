#pragma once
#include "DatabaseData.h"
#include "Utils/Memory.h"
#include "Utils/Buffer.h"

#include "Debugging/Log.h"

namespace Core
{
	class Response
	{
		friend class SQLInterface;
	public:
		Response() = default;
		Response(uint32_t id) : taskId(id) {}

		inline const uint32_t GetTaskId() const { return taskId; }
		inline const uint32_t GetDataCount() const { return data.size(); }
		inline bool HasData() const { return data.size() != 0; }
		inline std::vector< Ref<DatabaseData>>& GetData() { return data; }

		inline void AddData(Ref<DatabaseData> ref) { data.push_back(ref); }

		void Serialize(Ref<Buffer>& buffer) const
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

		void Deserialize(Ref<Buffer>& buffer)
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

		inline DatabaseData& operator[] (const int index) { return data[index].Get(); }
	private:
		std::vector<Ref<DatabaseData>> data;
		uint32_t taskId = 0;
	};
}