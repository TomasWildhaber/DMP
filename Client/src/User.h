#pragma once
#include "Team.h"
#include "Debugging/Log.h"

namespace Client
{
	class User
	{
		using TeamMap = std::unordered_map<int, Ref<Team>>;
	public:
		User() = default;
		User(uint32_t Id, const std::string& Name) : id(Id), name(Name) {} // light version of user for team list of users

		// Getters
		inline const uint32_t GetId() const { return id; }
		inline const char* GetName() const { return name.c_str(); }
		inline const bool HasTeams() const { return teams.size(); }
		inline const uint32_t GetTeamCount() const { return teams.size(); }
		inline TeamMap& GetTeams() { return teams; }

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

		// Team selection methods
		inline void SetSelectedTeam(Ref<Team>& team) { selectedTeamId = team->GetId(); }
		inline void UnselectTeam() { selectedTeamId = -1; }

		// Team vector wrappers
		inline const Ref<Team>& AddTeam(const Ref<Team>& team) { teams[team->GetId()] = team; return team; }
		inline void ClearTeams() { teams.clear(); }
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
		bool isAdmin = false;
		TeamMap teams;
		int selectedTeamId = -1; // keep selected team id, to save team selection after reallocation
	};
}