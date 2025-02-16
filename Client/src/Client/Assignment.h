#pragma once
#include "User.h"

namespace Client
{
	enum class AssignmentStatus
	{
		None = 0,
		InProgress,
		Submitted,
		Failed,
	};

	using UserMap = std::unordered_map<uint32_t, Ref<User>>;

	class Assignment
	{
	public:
		Assignment(uint32_t id, const char* name, const char* desc, const char* data_path, AssignmentStatus status, uint32_t rating, time_t deadline, time_t submitTime)
			: id(id), name(name), description(desc), status(status), deadline(deadline), submitTime(submitTime) {}

		inline const uint32_t GetId() const { return id; }
		inline UserMap& GetUsers() { return users; }
		inline const uint32_t GetUserCount() const { return users.size(); }

		inline const time_t GetDeadLine() const { return deadline; }
		inline const char* GetName() const { return name.c_str(); }
		inline const uint32_t GetNameSize() const { return name.size(); }
		inline const char* GetDescription() const { return description.c_str(); }
		inline const uint32_t GetDescriptionSize() const { return description.size(); }

		void SetSubmitTime(time_t Time) { deadline = Time; }

		void AddUser(Ref<User> user) { users[user->GetId()] = user; }
		void RemoveUser(uint32_t id) { users.erase(id); }
		void ClearUsers() { users.clear(); }
	private:
		uint32_t id;
		UserMap users;
		std::string description;
		std::string name;
		time_t deadline;
		time_t submitTime;
		AssignmentStatus status = AssignmentStatus::None;
	};

	class AssignmentData
	{
	public:
		AssignmentData& operator=(const Ref<Assignment>& asssignment)
		{
			strncpy(name, asssignment->GetName(), sizeof(name) - 1);
			name[sizeof(name) - 1] = '\0';

			strncpy(description, asssignment->GetDescription(), sizeof(description) - 1);
			description[sizeof(description) - 1] = '\0';

			return *this;
		}

		inline char* GetDescription() { return description; }
		inline const uint32_t GetDescriptionSize() const { return sizeof(description); }
		inline char* GetName() { return name; }
		inline const uint32_t GetNameSize() const { return sizeof(name); }

		inline UserMap& GetUsers() { return users; }
		inline const uint32_t GetUserCount() const { return users.size(); }

		void AddUser(Ref<User> user) { users[user->GetId()] = user; }
		void RemoveUser(uint32_t id) { users.erase(id); }
	private:
		UserMap users;
		char name[26] = {};
		char description[256] = {};
	};
}