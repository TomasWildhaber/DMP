#pragma once
#include "Message.h"


namespace Client
{
	// User class declaration to avoid cyclic header include
	class User;

	class Team
	{
	public:
		// Constructor for intializing team id, owner id and team name
		Team(uint32_t Id, uint32_t OwnerId, const char* Name) : id(Id), ownerId(OwnerId), name(Name) {}

		// Getters
		inline const uint32_t GetId() const { return id; }
		inline const uint32_t GetOwnerId() const { return ownerId; }
		inline const char* GetName() const { return name.c_str(); }
		inline const bool HasMessages() const { return messages.size(); }

		// Message getters
		inline const uint32_t GetMessageCount() const { return messages.size(); }
		inline const std::vector<Ref<Message>>& GetMessages() const { return messages; }

		// Message vector wrappers
		inline void AddMessage(const Ref<Message>& message) { messages.push_back(message); }
		inline void ClearMessages() { messages.clear(); }

		// Users getters
		inline const uint32_t GetUserCount() const { return users.size(); }
		inline const std::vector<Ref<User>>& GetUsers() const { return users; }

		// Users vector wrappers
		inline void AddUser(const Ref<User>& user) { users.push_back(user); }
		inline void ClearUsers() { users.clear(); }
	private:
		uint32_t id = 0;
		uint32_t ownerId = 0;
		std::string name;

		std::vector<Ref<Message>> messages;
		std::vector<Ref<User>> users;
	};
}