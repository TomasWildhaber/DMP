#pragma once
#include "Team.h"
#include "Invite.h"
#include "Notification.h"
#include "Debugging/Log.h"

namespace Client
{
	// Assignment class declaration to avoid cyclic header include
	class Assignment;

	// Wrapper class for user from db
	class User
	{
		using TeamMap = std::unordered_map<int, Ref<Team>>;
		using AssignmentMap = std::unordered_map<uint32_t, Ref<Assignment>>;
	public:
		User() = default;
		User(uint32_t Id, const std::string& Name) : id(Id), name(Name) {} // light version of user for team list of users

		// Getters
		inline const uint32_t GetId() const { return id; }
		inline const char* GetName() const { return name.c_str(); }
		inline const char* GetEmail() const { return email.c_str(); }

		// Getters for arrays
		inline const bool HasTeams() const { return teams.size(); }
		inline const uint32_t GetTeamCount() const { return teams.size(); }
		inline TeamMap& GetTeams() { return teams; }

		inline const bool HasInvites() const { return invites.size(); }
		inline std::vector<Ref<Invite>>& GetInvites() { return invites; }

		inline const bool HasNotifications() const { return notifications.size(); }
		inline std::vector<Ref<Notification>>& GetNotifications() { return notifications; }

		inline const bool HasAssignments() const { return assignments.size(); }
		inline AssignmentMap& GetAssignments() { return assignments; }

		// Team methods
		inline const bool IsSelectedTeamValid() const { return isTeamIdValid(selectedTeamId); }
		inline const bool HasSelectedTeam() const { return selectedTeamId >= 0 && IsSelectedTeamValid(); }
		inline Team& GetSelectedTeam() { ASSERT(!HasSelectedTeam(), "No team selected!"); return teams.at(selectedTeamId).Get(); }
		inline const bool IsTeamOwner(Team& team) const { return team.GetOwnerId() == id; }

		inline const bool IsAdmin() const { return isAdmin; }

		// Setters
		inline void SetId(uint32_t Id) { id = Id; }
		inline void SetAdminPrivileges(bool Admin) { isAdmin = Admin; }
		inline void SetName(const std::string& Name) { name = Name; }
		inline void SetEmail(const std::string& Email) { email = Email; }

		// Team selection methods
		inline void SetSelectedTeam(Ref<Team>& team) { selectedTeamId = team->GetId(); }
		inline void UnselectTeam() { selectedTeamId = -1; }

		// Team map wrappers
		inline const Ref<Team>& AddTeam(const Ref<Team>& team) { teams[team->GetId()] = team; return team; }
		inline void ClearTeams() { teams.clear(); }

		// Invite array wrappers
		inline void AddInvite(const Ref<Invite>& invite) { invites.push_back(invite); }
		inline void ClearInvites() { invites.clear(); }

		// Notification array wrappers
		inline void AddNotification(const Ref<Notification>& notification) { notifications.push_back(notification); }
		inline void ClearNotifications() { notifications.clear(); }

		// Assignments array wrappers
		inline void AddAssignment(uint32_t id, const Ref<Assignment>& assignment) { assignments[id] = (assignment); }
		inline void ClearAssignments() { assignments.clear(); }
	private:
		// Method for checking if team id is still valid after recreating team vector
		inline const bool isTeamIdValid(uint32_t Id) const
		{
			for (auto&[id, team] : teams)
			{
				if (id == Id)
					return true;
			}

			return false;
		}

		uint32_t id;
		std::string name;
		std::string email;
		bool isAdmin = false;
		int selectedTeamId = -1; // keep selected team id, to save team selection after reallocation

		TeamMap teams;
		std::vector<Ref<Invite>> invites;
		std::vector<Ref<Notification>> notifications;

		AssignmentMap assignments;
	};
}