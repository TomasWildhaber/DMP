#pragma once

namespace Client
{
	class Message
	{
	public:
		Message(const char* author, const char* message) : authorName(author), content(message) {}

		inline const std::string& GetAuthorName() const { return authorName; }
		inline const std::string& GetContent() const { return content; }
		inline uint32_t GetContentSize() const { return content.size(); }
	private:
		std::string authorName;
		std::string content;
	};
}