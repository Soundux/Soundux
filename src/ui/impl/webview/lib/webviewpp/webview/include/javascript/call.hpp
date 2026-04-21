#pragma once
#include <cstdint>
#include <json.hpp>
#include <string>

namespace Webview
{
    struct FunctionCallRequest
    {
        std::uint32_t seq;
        std::string function;
        nlohmann::json params;
    };

    struct NativeCallResponse
    {
        std::uint32_t seq;
        nlohmann::json result;
    };
} // namespace Webview