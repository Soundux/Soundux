#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <queue>
#include <shared_mutex>
#include <thread>

namespace Soundux
{
    namespace Objects
    {
        template <typename UID = int> class ProcessingQueue
        {
            struct Item
            {
                std::shared_ptr<std::atomic<bool>> handled;
                std::function<void()> callback;
                UID identifier;
            };

          private:
            std::queue<Item> queue;
            std::mutex queueMutex;

            std::vector<UID> unhandled;
            std::shared_mutex unhandledMutex;

            std::condition_variable cv;
            std::atomic<bool> stop;
            std::thread handler;

            void handle()
            {
                std::unique_lock lock(queueMutex);
                while (!stop)
                {
                    cv.wait(lock, [&]() { return !queue.empty() || stop; });
                    while (!queue.empty())
                    {
                        auto front = std::move(queue.front());
                        queue.pop();

                        lock.unlock();

                        front.callback();
                        if (front.handled)
                        {
                            front.handled->store(true);
                        }
                        auto found = std::find(unhandled.begin(), unhandled.end(), front.identifier);
                        if (found != unhandled.end())
                        {
                            unhandled.erase(found);
                        }

                        lock.lock();
                    }
                }
            }

          public:
            void push_unique(const UID &identifier, const std::function<void()> &item)
            {
                {
                    std::shared_lock lock(unhandledMutex);
                    if (std::find(unhandled.begin(), unhandled.end(), identifier) != unhandled.end())
                    {
                        return;
                    }
                }

                std::unique_lock lock(queueMutex);
                std::unique_lock lock_(unhandledMutex);

                unhandled.emplace_back(identifier);

                queue.emplace(Item{nullptr, item, identifier});
                lock.unlock();
                cv.notify_one();
            }

            ProcessingQueue()
            {
                handler = std::thread([this] { handle(); });
            }
            ~ProcessingQueue()
            {
                stop = true;
                cv.notify_all();
                handler.join();
            }
        };
    } // namespace Objects
} // namespace Soundux