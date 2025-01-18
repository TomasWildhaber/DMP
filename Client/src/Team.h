#pragma once

namespace Client
{
	class Team
	{
	public:
		Team(uint32_t Id, const char* Name) : id(Id), name(Name) {}

		inline const uint32_t GetId() const { return id; }
		inline const char* GetName() const { return name.c_str(); }
	private:
		uint32_t id = 0;
		std::string name;
	};
}