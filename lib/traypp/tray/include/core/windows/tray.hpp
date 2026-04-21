#pragma once
#if defined(_WIN32)
#include <Windows.h>
#include <core/traybase.hpp>
#include <map>
#include <shellapi.h>

namespace Tray
{
    class Tray : public BaseTray
    {
        HWND hwnd = nullptr;
        HMENU menu = nullptr;
        WNDCLASSEX windowClass;
        NOTIFYICONDATA notifyData;

        std::vector<std::shared_ptr<char[]>> allocations;
        static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
        static std::map<HWND, std::reference_wrapper<Tray>> trayList;
        static HMENU construct(const std::vector<std::shared_ptr<TrayEntry>> &, Tray *parent, bool cleanup = false);

      public:
        ~Tray();
        Tray(std::string identifier, Icon icon);
        template <typename... T> Tray(std::string identifier, Icon icon, const T &...entries) : Tray(identifier, icon)
        {
            addEntries(entries...);
        }

        void run() override;
        void exit() override;
        void update() override;
    };
} // namespace Tray
#endif