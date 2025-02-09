#pragma once

namespace Client
{
	class Invite
	{
	public:
		Invite(uint32_t id, uint32_t teamId, const char* teamName) : id(id), teamId(teamId)
		{
			strcpy_s(this->teamName, (char*)teamName);
		}

		inline const char* GetTeamName() const { return teamName; }
		inline uint32_t GetTeamId() const { return teamId; }
		inline uint32_t GetId() const { return id; }
	private:
		char teamName[26];
		uint32_t teamId;
		uint32_t id;
	};
}