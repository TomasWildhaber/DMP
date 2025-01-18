#pragma once
#include "Utils/Memory.h"
#include "Utils/Buffer.h"

namespace Core
{
	enum class MessageType
	{
		Text = 0,
		RawBytes,
		Command,
		Response,
	};

	struct MessageHeader
	{
		MessageHeader() = default;

		MessageType Type = MessageType::Text;
		uint32_t SessionId;
		uint32_t Size;
	};

	struct MessageBody
	{
		MessageBody() = default;
		MessageBody(const Ref<Buffer>& buffer) : Content(buffer) {}

		Ref<Buffer> Content;
	};

	struct Message
	{
		inline const uint32_t GetSize() const { return sizeof(MessageHeader) + Header.Size; }
		inline const uint32_t GetSessionId() const { return Header.SessionId; }
		inline const MessageType GetType() const { return Header.Type; }

		template<typename T>
		void CreateBody(T& content)
		{
			Header.Size = sizeof(T);
			Body.Content = CreateRef<Buffer>(sizeof(T));
			*Body.Content->GetDataAs<T>() = content;
		}

		MessageHeader Header;
		MessageBody Body;
	};
}