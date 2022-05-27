#include "queue.hpp"

namespace Soundux::Objects
{
    void Queue::handle()
    {
        std::unique_lock lock(queueMutex);
        while (!stop)
        {
            cv.wait(lock, [&]() { return !queue.empty() || stop; });
            while (!queue.empty())
            {
                auto front = std::move(*queue.begin());

                lock.unlock();
                front.second();
                lock.lock();

                queue.erase(front.first);
            }
        }
    }

    void Queue::push_unique(std::uint64_t id, std::function<void()> function)
    {
        {
            std::lock_guard lock(queueMutex);
            if (queue.find(id) != queue.end())
            {
                return;
            }
        }

        std::unique_lock lock(queueMutex);
        queue.emplace(id, std::move(function));
        lock.unlock();

        cv.notify_one();
    }

    Queue::Queue()
    {
        handler = std::thread([this] { handle(); });
    }
    Queue::~Queue()
    {
        stop = true;
        cv.notify_all();
        handler.join();
    }
} // namespace Soundux::Objects