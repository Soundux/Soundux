#include <stdexcept>
#if defined(_WIN32)
#include <core/image.hpp>

Tray::Image::Image(HBITMAP image) : image(image) {}
Tray::Image::Image(const char *path) : Image(std::string(path)) {}
Tray::Image::Image(const std::string &path)
    : image(reinterpret_cast<HBITMAP>(LoadImage(nullptr, path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE)))
{
    if (image == nullptr)
    {
        throw std::runtime_error("Failed to load bitmap");
    }
}

Tray::Image::operator HBITMAP()
{
    return image;
}
#endif