#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <optional>
#include <queue>
#include <thread>

namespace Soundux
{
    namespace Objects
    {
        class Queue
        {
            struct Call
            {
                std::function<void()> function;
                std::optional<std::uint64_t> id;
            };

            std::mutex queueMutex;
            std::vector<Call> queue;

            std::condition_variable cv;
            std::atomic<bool> stop;
            std::thread handler;

          private:
            void handle();

          public:
            Queue();
            ~Queue();

            void push(std::function<void()>);
            void push_unique(std::uint64_t, std::function<void()>);
        };
    } // namespace Objects
} // namespace Soundux