#pragma once

namespace Client
{
	class Message
	{
	public:
		// Constructor for initializing author name and message content
		Message(const char* author, const char* message) : authorName(author), content(message) {}

		// Getters
		inline const std::string& GetAuthorName() const { return authorName; }
		inline const std::string& GetContent() const { return content; }
		inline uint32_t GetContentSize() const { return content.size(); }
	private:
		std::string authorName;
		std::string content;
	};
}