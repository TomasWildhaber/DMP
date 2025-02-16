#pragma once
#include "DatabaseData.h"
#include "Utils/Memory.h"
#include "Utils/Buffer.h"

namespace Core
{
	class CommandBase
	{
	public:
		CommandBase() = default;
		CommandBase(uint32_t id) : taskId(id) {}

		inline const uint32_t GetTaskId() const { return taskId; }
		inline const uint32_t GetDataCount() const { return data.size(); }
		inline bool HasData() const { return data.size() != 0; }
		inline std::vector<Ref<DatabaseData>>& GetData() { return data; }

		inline void AddData(Ref<DatabaseData> ref) { data.push_back(ref); }

		virtual void Serialize(Ref<Buffer>& buffer) const = 0;
		virtual void Deserialize(Ref<Buffer>& buffer) = 0;

		inline DatabaseData& operator[] (const int index) { return data[index].Get(); }
	protected:
		std::vector<Ref<DatabaseData>> data;
		uint32_t taskId = 0;
	};
}