#pragma once
#include "Message.h"

namespace Core
{
	class MessageQueue
	{
	public:
		MessageQueue() = default;
		MessageQueue(const MessageQueue&& other) = delete;

		void Add(Ref<Message> message);
		void Pop();
		void Clear();

		inline Message& Get() { std::scoped_lock lock(mutex); return queue.front().Get(); }

		inline const uint32_t GetCount() { std::scoped_lock lock(mutex); return queue.size(); }
	private:
		std::mutex mutex;
		std::deque<Ref<Message>> queue;
	};
}