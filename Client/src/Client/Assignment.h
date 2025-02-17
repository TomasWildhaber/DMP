#pragma once
#include "User.h"

namespace Client
{
	enum class AssignmentStatus
	{
		None = 0,
		InProgress,
		Submitted,
		Rated,
	};

	using UserMap = std::unordered_map<uint32_t, Ref<User>>;

	class Assignment
	{
	public:
		Assignment(uint32_t id, const char* name, const char* desc, const char* data_path, AssignmentStatus status, uint32_t rating, const char* rating_desc, time_t deadline, time_t submitTime)
			: id(id), name(name), description(desc), status(status), rating(rating), rating_description(rating_desc), deadline(deadline), submitTime(submitTime) {}

		inline const uint32_t GetId() const { return id; }
		inline UserMap& GetUsers() { return users; }
		inline const uint32_t GetUserCount() const { return users.size(); }

		inline const time_t GetDeadLine() const { return deadline; }
		inline const time_t GetSubmitTime() const { return submitTime; }
		inline const AssignmentStatus GetStatus() const { return status; }

		inline const char* GetName() const { return name.c_str(); }
		inline const uint32_t GetNameSize() const { return name.size(); }

		inline const char* GetDescription() const { return description.c_str(); }
		inline const uint32_t GetDescriptionSize() const { return description.size(); }

		inline const char* GetRatingDescription() const { return rating_description.c_str(); }
		inline const uint32_t GetRatingDescriptionSize() const { return rating_description.size(); }

		inline const uint32_t GetRating() const { return rating; }

		void SetSubmitTime(time_t Time) { deadline = Time; }

		void AddUser(Ref<User> user) { users[user->GetId()] = user; }
		void RemoveUser(uint32_t id) { users.erase(id); }
		void ClearUsers() { users.clear(); }
	private:
		uint32_t id;
		UserMap users;

		std::string name;
		std::string description;
		std::string rating_description;
		uint32_t rating;
		AssignmentStatus status = AssignmentStatus::None;

		time_t deadline;
		time_t submitTime;
	};

	struct AssignmentData
	{
		AssignmentData()
		{
			UpdateDeadLine();
		}

		AssignmentData& operator=(const Ref<Assignment>& asssignment)
		{
			users.clear();
			AssignmentId = asssignment->GetId();
			UpdateDeadLine();

			strncpy(Name, asssignment->GetName(), sizeof(Name) - 1);
			Name[sizeof(Name) - 1] = '\0';

			strncpy(Description, asssignment->GetDescription(), sizeof(Description) - 1);
			Description[sizeof(Description) - 1] = '\0';

			return *this;
		}

		inline const uint32_t GetDescriptionSize() const { return sizeof(Description); }
		inline const uint32_t GetNameSize() const { return sizeof(Name); }

		inline const uint32_t GetUserCount() const { return users.size(); }
		inline UserMap& GetUsers() { return users; }

		void AddUser(Ref<User> user) { users[user->GetId()] = user; }
		void RemoveUser(uint32_t id) { users.erase(id); }

		// Set deadline time to 23:59:59
		void UpdateDeadLine()
		{
			DeadLineTm.tm_hour = 23;
			DeadLineTm.tm_min = 59;
			DeadLineTm.tm_sec = 59;

			DeadLine = mktime(&DeadLineTm);
		}

		char Name[26] = {};
		char Description[256] = {};
		int Rating = 0;
		time_t DeadLine = time(nullptr);;
		tm DeadLineTm = *localtime(&DeadLine);;
		int AssignmentId = -1;
	private:
		UserMap users;
	};
}