#pragma once

#if defined(__linux__)
#include <core/linux/window.hpp>
#elif defined(_WIN32)
#include <core/windows/window.hpp>
#endif