#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <queue>
#include <thread>

namespace Soundux
{
    namespace Objects
    {
        class Queue
        {
            std::map<std::uint64_t, std::function<void()>> queue;
            std::mutex queueMutex;

            std::condition_variable cv;
            std::atomic<bool> stop;
            std::thread handler;

          private:
            void handle();

          public:
            Queue();
            ~Queue();

            void push_unique(std::uint64_t, std::function<void()>);
        };
    } // namespace Objects
} // namespace Soundux