#pragma once
#include <string>

namespace Webview
{
    struct Resource
    {
        const std::size_t size;
        const unsigned char *data;
    };
} // namespace Webview