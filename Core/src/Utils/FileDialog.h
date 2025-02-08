#pragma once

class FileDialog
{
	FileDialog() = delete;
public:
	static std::filesystem::path OpenFile(const char* filter);
	static std::filesystem::path SaveFile(const char* filter);
};