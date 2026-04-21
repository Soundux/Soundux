#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>

namespace Webview
{
    namespace Traits
    {
        template <typename T> struct func_traits : public func_traits<decltype(&T::operator())>
        {
        };
        template <typename ClassType, typename ReturnType, typename... Args>
        struct func_traits<ReturnType (ClassType::*)(Args...) const>
        {
            enum
            {
                arg_count = sizeof...(Args)
            };
            using arg_t = std::tuple<std::decay_t<Args>...>;
            using return_t = ReturnType;
        };
        template <typename T> struct is_optional
        {
          private:
            static std::uint8_t test(...);
            template <typename O> static auto test(std::optional<O> *) -> std::uint16_t;

          public:
            static const bool value = sizeof(test(reinterpret_cast<T *>(0))) == sizeof(std::uint16_t);
        };
        template <typename T> struct is_shared_ptr
        {
          private:
            static std::uint8_t test(...);
            template <typename O> static auto test(std::shared_ptr<O> *) -> std::uint16_t;

          public:
            static const bool value = sizeof(test(reinterpret_cast<T *>(0))) == sizeof(std::uint16_t);
        };

        template <typename T, typename Tuple> struct has_type;
        template <typename T> struct has_type<T, std::tuple<>> : std::false_type
        {
        };
        template <typename T, typename U, typename... Ts>
        struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>>
        {
        };
        template <typename T, typename... Ts> struct has_type<T, std::tuple<T, Ts...>> : std::true_type
        {
        };

        template <typename T, typename... O>
        constexpr auto ignoreFirstImpl([[maybe_unused]] const std::tuple<T, O...> &tuple)
        {
            return std::tuple<O...>();
        }
        template <typename T> using ignore_first = decltype(ignoreFirstImpl(std::declval<T>()));
    } // namespace Traits
} // namespace Webview