#pragma once
#include "Team.h"
#include "Debugging/Log.h"

namespace Client
{
	class User
	{
	public:
		inline const uint32_t GetId() const { return id; }
		inline const char* GetName() const { return name.c_str(); }
		inline const bool HasTeams() const { return teams.size(); }
		inline const uint32_t GetTeamCount() const { return teams.size(); }
		inline std::vector<Ref<Team>>& GetTeams() { return teams; }

		inline const bool HasSelectedTeam() const { return selectedTeam.IsValid(); }
		inline Team& GetSelectedTeam() { ASSERT(!selectedTeam, "No team selected!"); return *selectedTeam; }

		inline const bool IsAdmin() const { return isAdmin; }

		inline void SetId(uint32_t Id) { id = Id; }
		inline void SetAdminPrivileges(bool Admin) { isAdmin = Admin; }
		inline void SetName(const std::string& Name) { name = Name; }

		inline void SetSelectedTeam(Ref<Team>& team) { selectedTeam = team; }
		inline void UnselectTeam() { selectedTeam = WeakRef<Team>(); }

		inline void AddTeam(const Ref<Team>& team) { teams.push_back(team); }
		inline void ClearTeams() { teams.clear(); }
	private:
		uint32_t id;
		std::string name;
		bool isAdmin = false;
		std::vector<Ref<Team>> teams;
		WeakRef<Team> selectedTeam;
	};
}