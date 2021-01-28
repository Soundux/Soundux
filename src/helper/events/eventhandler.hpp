#pragma once
#include <any>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace Soundux
{
    namespace traits
    {
        template <typename T> struct lambda_traits : public lambda_traits<decltype(&T::operator())>
        {
        };
        template <typename ClassType, typename ReturnType, typename... Args>
        struct lambda_traits<ReturnType (ClassType::*)(Args...) const>
        {
            using function_t = std::function<ReturnType(Args...)>;
            template <std::size_t i> using arg = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };
    } // namespace traits
    namespace sfinae
    {
        template <typename T> struct isLambda
        {
          private:
            static auto test(...) -> std::uint8_t;
            template <typename O> static auto test(O *) -> decltype(&O::operator(), std::uint16_t{});

          public:
            static const bool value = sizeof(test(reinterpret_cast<T *>(0))) == sizeof(std::uint16_t);
        };
    } // namespace sfinae
    namespace Objects
    {
        class EventHandler
        {
            struct EmittedEvent
            {
                std::string event;
                std::shared_ptr<std::atomic<bool>> handled;
                std::function<void(const std::any &)> caller;
            };

          private:
            std::thread handler;
            std::atomic<bool> kill = false;
            std::condition_variable_any cond;
            std::recursive_mutex emittedMutex;

            std::deque<EmittedEvent> emittedEvents;

            std::recursive_mutex eventsMutex;
            std::map<std::string, std::vector<std::any>> events;

          public:
            template <typename... T> void registerEvent(const std::string &name)
            {
                std::lock_guard<std::recursive_mutex> lock(eventsMutex);
                if (events.find(name) == events.end())
                {
                    events.insert({name, {}});
                }
                else
                {
                    throw std::runtime_error("Event already exists");
                }
            }
            void removeEvent(const std::string &name)
            {
                std::lock_guard<std::recursive_mutex> lock(eventsMutex);
                if (events.find(name) != events.end())
                {
                    events.erase(name);
                }
                else
                {
                    throw std::runtime_error("Event doesnt exists");
                }
            }
            template <typename T, std::enable_if_t<sfinae::isLambda<T>::value> * = nullptr>
            void registerCallback(const std::string &event, const T &callback)
            {
                std::lock_guard<std::recursive_mutex> lock(eventsMutex);
                using lambda_t = traits::lambda_traits<T>;

                typename lambda_t::function_t func = callback;
                events.at(event).push_back(func);
            }
            template <typename... T> auto emit(const std::string &event, T... args)
            {
                std::lock_guard<std::recursive_mutex> lock(emittedMutex);
                {
                    std::lock_guard<std::recursive_mutex> lock(eventsMutex);
                    if (events.find(event) == events.end())
                    {
                        throw std::runtime_error("Event doesnt exist");
                    }
                }
                using function_t = std::function<void(T...)>;
                auto handled = std::make_shared<std::atomic<bool>>(false);

                emittedEvents.push_back({event, handled, [args...](const std::any &any) {
                                             const auto &fun = std::any_cast<function_t>(any);
                                             fun(args...);
                                         }});
                cond.notify_one();
                return handled;
            }
            template <typename... T> void emitBlocking(const std::string &event, T... args)
            {
                {
                    std::lock_guard<std::recursive_mutex> lock(eventsMutex);
                    if (events.find(event) == events.end())
                    {
                        throw std::runtime_error("Event doesnt exist");
                    }
                }
                using function_t = std::function<void(T...)>;
                auto handled = std::make_shared<std::atomic<bool>>(false);
                {
                    std::lock_guard<std::recursive_mutex> lock(emittedMutex);
                    emittedEvents.push_back({event, handled, [args...](const std::any &any) {
                                                 const auto &fun = std::any_cast<function_t>(any);
                                                 fun(args...);
                                             }});
                }

                cond.notify_one();

                while (!*handled)
                    ;
            }
            template <typename... T> auto emitSilent(const std::string &event, T... args)
            {
                std::lock_guard<std::recursive_mutex> lock(emittedMutex);
                {
                    std::lock_guard<std::recursive_mutex> lock(eventsMutex);
                    if (events.find(event) == events.end())
                    {
                        throw std::runtime_error("Event doesnt exist");
                    }
                }
                using function_t = std::function<void(T...)>;
                auto handled = std::make_shared<std::atomic<bool>>(false);

                emittedEvents.push_back({event, handled, [args...](const std::any &any) {
                                             const auto &fun = std::any_cast<function_t>(any);
                                             fun(args...);
                                         }});

                return handled;
            }
            void init()
            {
                handler = std::thread([&] {
                    std::unique_lock<std::recursive_mutex> emittedLock(emittedMutex);
                    while (!kill)
                    {
                        cond.wait(emittedLock, [&] { return !emittedEvents.empty() || kill; });
                        {
                            std::lock_guard<std::recursive_mutex> eLock(eventsMutex);
                            while (!emittedEvents.empty())
                            {
                                const auto &event = emittedEvents.front();
                                if (events.find(event.event) != events.end())
                                {
                                    for (const auto &listener : events[event.event])
                                    {
                                        event.caller(listener);
                                        *event.handled = true;
                                    }
                                }
                                else
                                {
                                    throw std::runtime_error("Event doesnt exist");
                                }
                                emittedEvents.pop_front();
                            }
                        }
                    }
                });
            }
            void destroy()
            {
                kill = true;
                cond.notify_one();
                handler.join();
            }
        };
    } // namespace Objects
} // namespace Soundux