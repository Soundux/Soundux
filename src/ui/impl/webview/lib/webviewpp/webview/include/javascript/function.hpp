#pragma once
#include <functional>
#include <future>
#include <javascript/promise.hpp>
#include <json.hpp>
#include <misc/helpers.hpp>
#include <misc/traits.hpp>
#include <regex>
#include <string>

namespace Webview
{
    class Promise;
    class BaseWindow;

    class Function
    {
      protected:
        std::string name;
        std::function<nlohmann::json(const nlohmann::json &)> parserFunction;

      public:
        Function() = default;
        virtual ~Function() = default;
        template <typename func_t> Function(std::string name, const func_t &function) : name(std::move(name))
        {
            using func_traits = Traits::func_traits<func_t>;
            using rtn_t = typename func_traits::return_t;
            using arg_t = typename func_traits::arg_t;

            // NOLINTNEXTLINE
            parserFunction = [this, function](const nlohmann::json &j) -> nlohmann::json {
                arg_t packedArgs;
                Helpers::setTuple(packedArgs, [&j](auto index, auto &val) {
                    if (j.size() > index)
                    {
                        if (!j.at(index).is_null())
                        {
                            if constexpr (Traits::is_optional<std::decay_t<decltype(val)>>::value ||
                                          Traits::is_shared_ptr<std::decay_t<decltype(val)>>::value)
                            {
                                val =
                                    j.at(index)
                                        .template get<std::decay_t<typename std::decay_t<decltype(val)>::value_type>>();
                            }
                            else
                            {
                                j.at(index).get_to(val);
                            }
                        }
                    }
                });

                if constexpr (std::is_void_v<rtn_t>)
                {
                    //* If the function return type is void we just need to call it and ignore the return type
                    auto unpack = [function](auto &&...args) { function(args...); };
                    std::apply(unpack, packedArgs);
                }
                else
                {
                    //* In case the return type is nothing special, we just want to save the return value of the
                    //* function to rtn and then convert rtn to json

                    rtn_t rtn;

                    auto unpack = [&rtn, function](auto &&...args) { rtn = std::move(function(args...)); };
                    std::apply(unpack, packedArgs);

                    if constexpr (Traits::is_optional<rtn_t>::value || Traits::is_shared_ptr<rtn_t>::value)
                    {
                        //* Just to make sure this wont break with optionals
                        if (rtn)
                        {
                            return nlohmann::json(*rtn);
                        }
                    }
                    else
                    {
                        return nlohmann::json(rtn);
                    }
                }

                return nullptr;
            };
        }

        std::string getName() const;
        std::function<nlohmann::json(const nlohmann::json &)> getFunc() const;
    };

    class AsyncFunction : public Function
    {
      protected:
        std::function<void(BaseWindow &, const nlohmann::json &, std::uint32_t)> parserFunction;

      public:
        template <typename func_t> AsyncFunction(std::string name, const func_t &function) : Function()
        {
            this->name = std::move(name);

            using func_traits = Traits::func_traits<func_t>;
            using rtn_t = typename func_traits::return_t;
            using arg_t = typename func_traits::arg_t;

            static_assert(std::is_same_v<rtn_t, void>);
            static_assert(std::is_same_v<std::decay_t<decltype(std::get<0>(std::declval<arg_t>()))>, Promise>);

            // NOLINTNEXTLINE
            parserFunction = [this, function](BaseWindow &parent, const nlohmann::json &j, std::uint32_t seq) {
                //* Return type is always void
                Traits::ignore_first<arg_t> packedArgs;
                Helpers::setTuple(packedArgs, [&j](auto index, auto &val) {
                    if (j.size() > index)
                    {
                        j.at(index).get_to(val);
                    }
                });

                auto unpack = [&parent, function, seq](auto &&...args) { function(Promise(parent, seq), args...); };
                std::apply(unpack, packedArgs);
            };
        }

        ~AsyncFunction() override = default;
        std::function<void(BaseWindow &, const nlohmann::json &, std::uint32_t)> getFunc() const;
    };

    class JavaScriptFunction
    {

        std::string name;

        std::mutex resultMutex;
        std::promise<nlohmann::json> result;

        std::mutex argumentsMutex;
        std::vector<nlohmann::json> arguments;

      public:
        template <typename... T> JavaScriptFunction(std::string name, const T &...params) : name(std::move(name))
        {
            std::lock_guard guard(argumentsMutex);
            auto unpack = [this](auto &&arg) { arguments.emplace_back(nlohmann::json(arg)); };
            (unpack(params), ...);
        }
        JavaScriptFunction(JavaScriptFunction &);

        std::string getName() const;
        std::vector<nlohmann::json> getArguments() const;

        void resolve(const nlohmann::json &);
        std::shared_future<nlohmann::json> getResult();
    };
} // namespace Webview