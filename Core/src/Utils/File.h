#pragma once
#include "Utils/Memory.h"
#include "Utils/Buffer.h"

class File
{
public:
	File() = default; // Default constructor for deserialization
	File(int id, const char* name, bool byuser) : id (id), name(name), byUser(byuser) {} // Constructor without data
	File(const char* name, Ref<Buffer> data, bool byuser) : name(name), dataBuffer(data), byUser(byuser) {} // Constructor for full initialization

	const char* GetName() const { return name.c_str(); }
	inline const int GetId() const { return id; }
	inline const bool IsByUser() const { return byUser; }
	inline Ref<Buffer>& GetData() { return dataBuffer; }

	inline void SetId(int Id) { id = Id; }

	void Serialize(Ref<Buffer>& buffer) const
	{
		std::ostringstream os;

		size_t nameSize = name.size();
		uint32_t bufferSize = dataBuffer->GetSize();

		os.write(reinterpret_cast<const char*>(&id), sizeof(int));
		os.write(reinterpret_cast<const char*>(&byUser), sizeof(bool));
		os.write(reinterpret_cast<const char*>(&nameSize), sizeof(size_t));
		os.write(name.c_str(), name.size());

		os.write(reinterpret_cast<const char*>(&bufferSize), sizeof(uint32_t));

		os.write(dataBuffer->GetData(), dataBuffer->GetSize());

		buffer = CreateRef<Buffer>(os.str().size());
		buffer->Write(os.str().data(), os.str().size());
	}

	void Deserialize(Ref<Buffer>& buffer)
	{
		std::istringstream is(std::string(buffer->GetData(), buffer->GetSize()));

		size_t nameSize;
		uint32_t bufferSize;

		is.read(reinterpret_cast<char*>(&id), sizeof(int));
		is.read(reinterpret_cast<char*>(&byUser), sizeof(bool));
		is.read(reinterpret_cast<char*>(&nameSize), sizeof(size_t));

		std::string nameBuffer(nameSize, 0);
		is.read(&nameBuffer[0], nameSize);
		name = nameBuffer;

		is.read(reinterpret_cast<char*>(&bufferSize), sizeof(uint32_t));
		dataBuffer = new Buffer(bufferSize);
		is.read(dataBuffer->GetData(), dataBuffer->GetSize());
	}

	void DeserializeWithoutData(Ref<Buffer>& buffer)
	{
		std::istringstream is(std::string(buffer->GetData(), buffer->GetSize()));

		size_t nameSize;

		is.read(reinterpret_cast<char*>(&id), sizeof(int));
		is.read(reinterpret_cast<char*>(&byUser), sizeof(bool));
		is.read(reinterpret_cast<char*>(&nameSize), sizeof(size_t));

		std::string nameBuffer(nameSize, 0);
		is.read(&nameBuffer[0], nameSize);
		name = nameBuffer;
	}
private:
	bool byUser;
	int id = -1; // Used either for assignment_id
	std::string name;
	Ref<Buffer> dataBuffer;
};