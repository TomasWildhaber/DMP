#pragma once

class Buffer
{
public:
	Buffer() = default;
	Buffer(uint32_t Size);

	void Allocate(uint32_t Size);
	void Release();
	void WriteZeros();
	void Write(const void* data, uint32_t size);

	template<typename T>
	inline T* GetDataAs() { return (T*)data; }
	inline char* GetData() { return (char*)data; }

	inline const uint32_t GetSize() const { return size; }
	inline const bool IsEmpty() const { return !*data; }

	operator bool() const { return (bool)data; }
private:
	uint8_t* data = nullptr;
	uint32_t size = 0;
};