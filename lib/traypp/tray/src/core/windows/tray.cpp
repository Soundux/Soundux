#if defined(_WIN32)
#include <Windows.h>
#include <core/entry.hpp>
#include <core/windows/tray.hpp>
#include <stdexcept>

#include <components/button.hpp>
#include <components/imagebutton.hpp>
#include <components/label.hpp>
#include <components/separator.hpp>
#include <components/submenu.hpp>
#include <components/syncedtoggle.hpp>
#include <components/toggle.hpp>

static constexpr auto WM_TRAY = WM_USER + 1;
std::map<HWND, std::reference_wrapper<Tray::Tray>> Tray::Tray::trayList;

Tray::Tray::Tray(std::string identifier, Icon icon) : BaseTray(std::move(identifier), icon)
{
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = wndProc;
    windowClass.lpszClassName = this->identifier.c_str();
    windowClass.hInstance = GetModuleHandle(nullptr);

    if (RegisterClassEx(&windowClass) == 0)
    {
        throw std::runtime_error("Failed to register class");
    }

    // NOLINTNEXTLINE
    hwnd = CreateWindow(this->identifier.c_str(), nullptr, 0, 0, 0, 0, 0, nullptr, nullptr, windowClass.hInstance,
                        nullptr);
    if (hwnd == nullptr)
    {
        throw std::runtime_error("Failed to create window");
    }

    if (UpdateWindow(hwnd) == 0)
    {
        throw std::runtime_error("Failed to update window");
    }

    memset(&notifyData, 0, sizeof(NOTIFYICONDATA));
    notifyData.cbSize = sizeof(NOTIFYICONDATA);
    notifyData.hWnd = hwnd;
    notifyData.uFlags = NIF_ICON | NIF_MESSAGE;
    notifyData.uCallbackMessage = WM_TRAY;
    notifyData.hIcon = this->icon;

    if (Shell_NotifyIcon(NIM_ADD, &notifyData) == FALSE)
    {
        throw std::runtime_error("Failed to register tray icon");
    }
    trayList.insert({hwnd, *this});
}
Tray::Tray::~Tray()
{
    allocations.clear();
}
void Tray::Tray::exit()
{
    Shell_NotifyIcon(NIM_DELETE, &notifyData);
    DestroyIcon(notifyData.hIcon);
    DestroyMenu(menu);

    UnregisterClass(identifier.c_str(), GetModuleHandle(nullptr));
    PostMessage(hwnd, WM_QUIT, 0, 0);
    allocations.clear();

    DestroyIcon(notifyData.hIcon);
    trayList.erase(hwnd);
}

void Tray::Tray::update()
{
    DestroyMenu(menu);
    menu = construct(entries, this, true);

    if (Shell_NotifyIcon(NIM_MODIFY, &notifyData) == FALSE)
    {
        throw std::runtime_error("Failed to update tray icon");
    }
    SendMessage(hwnd, WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(menu), 0);
}

HMENU Tray::Tray::construct(const std::vector<std::shared_ptr<TrayEntry>> &entries, Tray *parent, bool cleanup)
{
    static auto id = 0;
    if (cleanup)
    {
        parent->allocations.clear();
    }

    auto *menu = CreatePopupMenu();
    for (const auto &entry : entries)
    {
        auto *item = entry.get();

        auto name = std::shared_ptr<char[]>(new char[item->getText().size() + 1]);
        strcpy(name.get(), item->getText().c_str()); // NOLINT
        parent->allocations.emplace_back(name);

        MENUITEMINFO winItem{0};

        winItem.wID = ++id;
        winItem.dwTypeData = name.get();
        winItem.cbSize = sizeof(MENUITEMINFO);
        winItem.fMask = MIIM_TYPE | MIIM_STATE | MIIM_DATA | MIIM_ID;
        winItem.dwItemData = reinterpret_cast<ULONG_PTR>(item);

        if (auto *toggle = dynamic_cast<Toggle *>(item); toggle)
        {
            if (toggle->isToggled())
            {
                winItem.fState |= MFS_CHECKED;
            }
            else
            {
                winItem.fState |= MFS_UNCHECKED;
            }
        }
        else if (auto *syncedToggle = dynamic_cast<SyncedToggle *>(item); syncedToggle)
        {
            if (syncedToggle->isToggled())
            {
                winItem.fState |= MFS_CHECKED;
            }
            else
            {
                winItem.fState |= MFS_UNCHECKED;
            }
        }
        else if (auto *submenu = dynamic_cast<Submenu *>(item); submenu)
        {
            winItem.fMask |= MIIM_SUBMENU;
            winItem.hSubMenu = construct(submenu->getEntries(), parent);
        }
        else if (auto *iconButton = dynamic_cast<ImageButton *>(item); iconButton)
        {
            winItem.fMask = MIIM_STRING | MIIM_BITMAP | MIIM_FTYPE | MIIM_STATE;
            winItem.hbmpItem = iconButton->getImage();
        }
        else if (dynamic_cast<Label *>(item))
        {
            winItem.fState = MFS_DISABLED;
        }
        else if (dynamic_cast<Separator *>(item))
        {
            winItem.fType = MFT_SEPARATOR;
        }

        if (!dynamic_cast<Label *>(item))
        {
            if (item->isDisabled())
            {
                winItem.fState = MFS_DISABLED;
            }
        }

        InsertMenuItem(menu, id, TRUE, &winItem);
    }

    return menu;
}

LRESULT CALLBACK Tray::Tray::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TRAY:
        if (lParam == WM_RBUTTONUP)
        {
            POINT p;
            GetCursorPos(&p);
            SetForegroundWindow(hwnd);
            auto &menu = trayList.at(hwnd).get();
            auto cmd = TrackPopupMenu(menu.menu, TPM_RETURNCMD | TPM_NONOTIFY, p.x, p.y, 0, hwnd, nullptr);
            SendMessage(hwnd, WM_COMMAND, cmd, 0);
            return 0;
        }
        break;
    case WM_COMMAND:
        MENUITEMINFO winItem{0};
        winItem.fMask = MIIM_DATA | MIIM_ID;
        winItem.cbSize = sizeof(MENUITEMINFO);

        auto &menu = trayList.at(hwnd).get();
        if (GetMenuItemInfo(menu.menu, static_cast<UINT>(wParam), FALSE, &winItem))
        {
            auto *item = reinterpret_cast<TrayEntry *>(winItem.dwItemData);
            if (auto *button = dynamic_cast<Button *>(item); button)
            {
                button->clicked();
            }
            else if (auto *toggle = dynamic_cast<Toggle *>(item); toggle)
            {
                toggle->onToggled();
                menu.update();
            }
            else if (auto *syncedToggle = dynamic_cast<SyncedToggle *>(item); syncedToggle)
            {
                syncedToggle->onToggled();
                menu.update();
            }
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Tray::Tray::run()
{
    static MSG msg;
    while (GetMessage(&msg, hwnd, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

#endif