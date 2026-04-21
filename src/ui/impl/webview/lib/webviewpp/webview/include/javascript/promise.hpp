#pragma once
#include <cstdint>
#include <json.hpp>
#include <misc/traits.hpp>

namespace Webview
{
    class BaseWindow;
    class Promise
    {
        std::uint32_t id;
        BaseWindow &parent;

      public:
        Promise(BaseWindow &parent, std::uint32_t id);

        void discard() const;
        void resolve(const nlohmann::json &) const;

        template <typename T> void resolve(const std::optional<T> &result)
        {
            if (result)
            {
                resolve(*result);
            }
            else
            {
                discard();
            }
        }
    };
} // namespace Webview