#include "pch.h"
#include "Database/Hash.h"
#include "bcrypt/BCrypt.hpp"

namespace Core
{
	std::string GenerateHash(const char* text)
	{
		return BCrypt::generateHash(text);
	}

	bool ValidateHash(const char* text, const char* hash)
	{
		return BCrypt::validatePassword(text, hash);
	}
}