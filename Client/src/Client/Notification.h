#pragma once

namespace Client
{
	// Wrapper class for notification from db
	class Notification
	{
	public:
		Notification(uint32_t id, const char* message) : id(id), message(message) {}

		// Getters
		inline const uint32_t GetId() const { return id; }
		inline const char* GetMessage() const { return message.c_str(); }
	private:
		std::string message;
		uint32_t id;
	};
}