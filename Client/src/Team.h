#pragma once
#include "Message.h"

namespace Client
{
	class Team
	{
	public:
		Team(uint32_t Id, const char* Name) : id(Id), name(Name) {}

		inline const uint32_t GetId() const { return id; }
		inline const char* GetName() const { return name.c_str(); }
		inline const bool HasMessages() const { return messages.size(); }

		inline const uint32_t GetMessageCount() const { return messages.size(); }
		inline const std::vector<Ref<Message>>& GetMessages() const { return messages; }

		inline void AddMessage(const Ref<Message>& message) { messages.push_back(message); }
		inline void ClearMessages() { messages.clear(); }
	private:
		uint32_t id = 0;
		std::string name;
		std::vector<Ref<Message>> messages;
	};
}