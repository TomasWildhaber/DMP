#pragma once
#include "Utils/Buffer.h"
#include "Utils/Memory.h"

#include <filesystem>

class FileReader
{
public:
    static inline uint32_t MaxFileSize = 20 * 1024 * 1024; // 20MB

	static Ref<Buffer> ReadFile(std::filesystem::path path)
	{
        bool error = false;
		std::ifstream stream(path, std::ios::binary);

        if (!stream)
            error = true;

        // Read file size
        stream.seekg(0, std::ios::end);
        std::streamsize fileSize = stream.tellg();
        stream.seekg(0, std::ios::beg);

        // Ensure max file size is 20MB
        if (MaxFileSize < fileSize)
            return Ref<Buffer>();

        Ref<Buffer> buffer = new Buffer(fileSize);

        // Read file and store it's content into buffer
        if (!stream.read(buffer->GetData(), buffer->GetSize()))
            error = true;

        stream.close();

        if (error)
        {
            ERROR("Failed to read file!");
            return Ref<Buffer>();
        }

        return buffer;
	}
};