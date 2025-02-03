#pragma once

namespace Core
{
	// Bcrypt wrapper functions
	std::string GenerateHash(const char* text);
	bool ValidateHash(const char* text, const char* hash);
}