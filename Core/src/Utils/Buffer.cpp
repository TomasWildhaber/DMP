#include "pch.h"
#include "Buffer.h"

Buffer::Buffer(uint32_t Size)
{
	Allocate(Size);
}

void Buffer::Allocate(uint32_t Size)
{
	Release();

	size = Size;
	data = (uint8_t*)malloc(size);

	WriteZeros();
}

void Buffer::Release()
{
	free(data);
	data = nullptr;
	size = 0;
}

void Buffer::WriteZeros()
{
	memset(data, 0, size);
}