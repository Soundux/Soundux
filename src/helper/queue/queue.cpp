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
                auto front = queue.begin();
                front->function();
                queue.erase(front);
            }
        }
    }
    void Queue::push_unique(std::uint64_t id, std::function<void()> function)
    {
        {
            std::lock_guard lock(queueMutex);
            if (std::find_if(queue.begin(), queue.end(),
                             [&id](const auto &entry) { return entry.id && *entry.id == id; }) != queue.end())
            {
                return;
            }
        }

        std::unique_lock lock(queueMutex);
        queue.emplace_back(Call{std::move(function), id});
        lock.unlock();

        cv.notify_one();
    }
    void Queue::push(std::function<void()> function)
    {
        std::unique_lock lock(queueMutex);
        queue.emplace_back(Call{std::move(function), std::nullopt});
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