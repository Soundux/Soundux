#if defined(_WIN32)
#include <core/icon.hpp>

Tray::Icon::Icon(const std::string &path)
    : hIcon(reinterpret_cast<HICON>(
          LoadImageA(nullptr, path.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE)))
{
}
Tray::Icon::Icon(HICON icon) : hIcon(icon) {}
Tray::Icon::Icon(const char *path) : Icon(std::string(path)) {}
Tray::Icon::Icon(WORD icon) : hIcon(LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(icon))) {} // NOLINT

Tray::Icon::operator HICON()
{
    return hIcon;
}

#endif