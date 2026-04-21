#pragma once
#include <components/button.hpp>
#include <components/imagebutton.hpp>
#include <components/label.hpp>
#include <components/separator.hpp>
#include <components/submenu.hpp>
#include <components/syncedtoggle.hpp>
#include <components/toggle.hpp>

#if defined(_WIN32)
#include <core/windows/tray.hpp>
#elif defined(__linux__)
#include <core/linux/tray.hpp>
#endif