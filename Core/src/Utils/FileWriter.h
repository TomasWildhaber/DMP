#pragma once
#include "Utils/Buffer.h"
#include "Utils/Memory.h"

#include <filesystem>

class FileWriter
{
public:
	static bool WriteFile(std::filesystem::path path, Ref<Buffer>& data)
	{
		bool success = true;
		std::ofstream stream(path, std::ios::binary);

		if (!stream)
			success = false;

		if (!stream.write(data->GetData(), data->GetSize()))
			success = false;

		stream.close();
		return success;
	}
};