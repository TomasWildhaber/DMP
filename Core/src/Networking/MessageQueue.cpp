#include "pch.h"
#include "MessageQueue.h"

namespace Core
{
	void MessageQueue::Add(Ref<Message> message)
	{
		std::scoped_lock lock(mutex);
		queue.push_back(message);
	}

	void MessageQueue::Pop()
	{
		std::scoped_lock lock(mutex);
		queue.pop_front();
	}

	void MessageQueue::Clear()
	{
		std::scoped_lock lock(mutex);
		queue.clear();
	}
}