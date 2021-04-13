#include "webview.hpp"
#include <core/global/globals.hpp>
#include <cstdint>
#include <fancy.hpp>
#include <filesystem>
#include <helper/json/bindings.hpp>
#include <helper/systeminfo/systeminfo.hpp>
#include <helper/version/check.hpp>
#include <helper/ytdl/youtube-dl.hpp>
#include <optional>
#include <thread>

#ifdef _WIN32
#include "../../assets/icon.h"
#include <shellapi.h>
#include <windows.h>
#endif

namespace Soundux::Objects
{
    void WebView::setup()
    {
        Window::setup();

        webview.setup(Soundux::Globals::gData.width, Soundux::Globals::gData.height);
        webview.setTitle("Soundux");
        webview.enableDevTools(std::getenv("SOUNDUX_DEBUG") != nullptr); // NOLINT

#ifdef _WIN32
        char rawPath[MAX_PATH];
        GetModuleFileNameA(nullptr, rawPath, MAX_PATH);

        auto path = std::filesystem::canonical(rawPath).parent_path() / "dist" / "index.html";
        tray = std::make_shared<Tray>("soundux-tray", IDI_ICON1);
#endif
#if defined(__linux__)
        auto path = std::filesystem::canonical("/proc/self/exe").parent_path() / "dist" / "index.html";
        std::filesystem::path iconPath;

        if (std::filesystem::exists("/app/share/icons/hicolor/256x256/apps/io.github.Soundux.png"))
        {
            iconPath = "/app/share/icons/hicolor/256x256/apps/io.github.Soundux.png";
        }
        else if (std::filesystem::exists("/usr/share/pixmaps/soundux.png"))
        {
            iconPath = "/usr/share/pixmaps/soundux.png";
        }
        else
        {
            Fancy::fancy.logTime().warning() << "Failed to find iconPath for tray icon" << std::endl;
        }

        tray = std::make_shared<Tray>("soundux-tray", iconPath);
#endif

        webview.addCallback("getSettings", []() { return Globals::gSettings; });
        webview.addCallback("isLinux", []() {
#if defined(__linux__)
            return true;
#else
            return false;
#endif
        });
        webview.addCallback("addTab", [this]() { return (addTab()); });
        webview.addCallback("getTabs", []() { return Globals::gData.getTabs(); });
        webview.addCallback("getPlayingSounds", []() { return Globals::gAudio.getPlayingSounds(); });
        webview.addCallback("playSound", [this](std::uint32_t id) { return playSound(id); });
        webview.addCallback("stopSound", [this](std::uint32_t id) { return stopSound(id); });
        webview.addCallback("seekSound",
                            [this](std::uint32_t id, std::uint64_t seekTo) { return seekSound(id, seekTo); });
        webview.addCallback("pauseSound", [this](std::uint32_t id) { return pauseSound(id); });
        webview.addCallback("resumeSound", [this](std::uint32_t id) { return resumeSound(id); });
        webview.addCallback("repeatSound", [this](std::uint32_t id, bool repeat) { return repeatSound(id, repeat); });
        webview.addCallback("stopSounds", [this]() { return stopSounds(); });
        webview.addCallback("changeSettings",
                            [this](const Settings &newSettings) { return changeSettings(newSettings); });
        webview.addCallback("requestHotkey", [](bool state) { Globals::gHotKeys.shouldNotify(state); });
        webview.addCallback("setHotkey",
                            [this](std::uint32_t id, const std::vector<int> &keys) { return setHotkey(id, keys); });
        webview.addCallback("getHotkeySequence",
                            [this](const std::vector<int> &keys) { return getHotkeySequence(keys); });
        webview.addCallback("removeTab", [this](std::uint32_t id) { return removeTab(id); });
        webview.addCallback("refreshTab", [this](std::uint32_t id) { return refreshTab(id); });
        webview.addCallback("moveTabs", [this](const std::vector<int> &newOrder) { return changeTabOrder(newOrder); });
        webview.addCallback("markFavorite",
                            [this](const std::uint32_t &id, bool favourite) { return markFavourite(id, favourite); });
        webview.addCallback("getFavorites", [this] { return getFavourites(); });
        webview.addCallback("isYoutubeDLAvailable", []() { return Globals::gYtdl.available(); });
        webview.addCallback("getYoutubeDLInfo", [this](const std::string &url, const JSPromise &promise) {
            std::thread fetchInfo([url, promise, this] { webview.resolve(promise, Globals::gYtdl.getInfo(url)); });
            fetchInfo.detach();
        });
        webview.addCallback("startYoutubeDLDownload", [this](const std::string &url, const JSPromise &promise) {
            std::thread download([promise, url, this] { webview.resolve(promise, Globals::gYtdl.download(url)); });
            download.detach();
        });
        webview.addCallback("stopYoutubeDLDownload", [this](const JSPromise &promise) {
            std::thread killDownload([promise, this] {
                Globals::gYtdl.killDownload();
                webview.resolve(promise, "null");
            });
            killDownload.detach();
        });
        webview.addCallback("getSystemInfo", []() -> std::string { return SystemInfo::getSummary(); });
        webview.addCallback("updateCheck", [this](const JSPromise &promise) {
            std::thread updateCheck([promise, this] { webview.resolve(promise, VersionCheck::getStatus()); });
            updateCheck.detach();
        });
        webview.addCallback("isOnFavorites", [this](bool state) { isOnFavorites(state); });
        webview.addCallback("deleteSound", [this](std::uint32_t id) { deleteSound(id); });

#if !defined(__linux__)
        webview.addCallback("getOutputs", [this]() { return getOutputs(); });
#endif
#if defined(_WIN32)
        webview.addCallback("openUrl", [](const std::string &url) {
            ShellExecuteA(nullptr, nullptr, url.c_str(), nullptr, nullptr, SW_SHOW);
        });
#endif
#if defined(__linux__)
        webview.addCallback("openUrl", [](const std::string &url) {
            if (system(("xdg-open \"" + url + "\"").c_str()) != 0) // NOLINT
            {
                Fancy::fancy.logTime().warning() << "Failed to open url " << url << std::endl;
            }
        });
        webview.addCallback("getOutputs", [this]() { return getOutputs(); });
        webview.addCallback("getPlayback", [this]() { return getPlayback(); });
        webview.addCallback("startPassthrough", [this](const std::string &app) { return startPassthrough(app); });
        webview.addCallback("stopPassthrough", [this]() { stopPassthrough(); });
        webview.addCallback("isSwitchOnConnectLoaded", []() { return Globals::gPulse.isSwitchOnConnectLoaded(); });
        webview.addCallback("unloadSwitchOnConnect", []() { Globals::gPulse.unloadSwitchOnConnect(); });
#endif
        webview.setCloseCallback([this]() { tray->getChildren().at(1)->setName(translations.show); });
        webview.setResizeCallback([](int width, int height) {
            Globals::gData.width = width;
            Globals::gData.height = height;
        });
        webview.setNavigateCallback([this]([[maybe_unused]] const std::string &url) {
            webview.callJS<std::string>("window.getTranslation", "settings.title")
                ->then([this](const std::string &result) {
                    translations.settings = result;
                    tray->update();
                });
            webview.callJS<std::string>("window.getTranslation", "settings.tabHotkeysOnly")
                ->then([this](const std::string &result) {
                    translations.tabHotkeys = result;
                    tray->update();
                });
            webview.callJS<std::string>("window.getTranslation", "settings.muteDuringPlayback")
                ->then([this](const std::string &result) {
                    translations.muteDuringPlayback = result;
                    tray->update();
                });
            webview.callJS<std::string>("window.getTranslation", "tray.show")->then([this](const std::string &result) {
                translations.show = result;
                tray->update();
            });
            webview.callJS<std::string>("window.getTranslation", "tray.hide")->then([this](const std::string &result) {
                translations.hide = result;
                tray->update();
            });
            webview.callJS<std::string>("window.getTranslation", "tray.exit")->then([this](const std::string &result) {
                translations.exit = result;
                tray->update();
            });

            webview.whenAllReady([&] {
                tray->addItem(TrayButton(translations.exit, [this]() {
                    tray->exit();
                    webview.exit();
                }));
                tray->addItem(TrayButton(translations.hide, [this]() {
                    if (!webview.getIsHidden())
                    {
                        webview.hide();
                        tray->getChildren().at(1)->setName(translations.show);
                    }
                    else
                    {
                        webview.show();
                        tray->getChildren().at(1)->setName(translations.hide);
                    }
                }));

                auto settings = tray->addItem(TraySubmenu(translations.settings));
                settings->addItems(
                    TraySyncedCheck(translations.muteDuringPlayback, Globals::gSettings.muteDuringPlayback,
                                    [this](bool state) {
                                        auto settings = Globals::gSettings;
                                        settings.muteDuringPlayback = state;
                                        changeSettings(settings);
                                    }),
                    TraySyncedCheck(translations.tabHotkeys, Globals::gSettings.tabHotkeysOnly, [this](bool state) {
                        auto settings = Globals::gSettings;
                        settings.tabHotkeysOnly = state;
                        changeSettings(settings);
                    }));
            });
        });

#if defined(IS_EMBEDDED)
#if defined(__linux__)
        webview.navigate("embedded://" + path.string());
#elif defined(_WIN32)
        webview.navigate("file:///embedded/" + path.string());
#endif
#else
        webview.navigate("file://" + path.string());
#endif
    }
    void WebView::mainLoop()
    {
        webview.run();
        tray->exit();
        Fancy::fancy.logTime() << "UI exited" << std::endl;
    }
    void WebView::onHotKeyReceived(const std::vector<int> &keys)
    {
        std::string hotkeySequence;
        for (const auto &key : keys)
        {
            hotkeySequence += Globals::gHotKeys.getKeyName(key) + " + ";
        }
        auto js = "window.hotkeyReceived(`" + hotkeySequence.substr(0, hotkeySequence.length() - 3) +
                  "`, JSON.parse(`" + nlohmann::json(keys).dump() + "`));";

        webview.runCodeSafe(js);
    }
    void WebView::onSoundFinished(const PlayingSound &sound)
    {
        Window::onSoundFinished(sound);
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.finishSound(JSON.parse(`" + soundObj + "`));";

        webview.runCodeSafe(js);
    }
    void WebView::onSoundPlayed(const PlayingSound &sound)
    {
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.onSoundPlayed(JSON.parse(`" + soundObj + "`));";

        webview.runCodeSafe(js);
    }
    void WebView::onSoundProgressed(const PlayingSound &sound)
    {
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.updateSound(JSON.parse(`" + soundObj + "`));";

        webview.runCodeSafe(js);
    }
    void WebView::onDownloadProgressed(float progress, const std::string &eta)
    {
        auto js = "window.downloadProgressed(" + std::to_string(progress) + ", `" + eta + "`);";
        webview.runCodeSafe(js);
    }
    void WebView::onError(const ErrorCode &error)
    {
        auto js = "window.onError(" + std::to_string(static_cast<std::uint8_t>(error)) + ");";
        webview.runCodeSafe(js);
    }
    void WebView::changeSettings(const Settings &newSettings)
    {
        Window::changeSettings(newSettings);

        webview.hideOnClose(newSettings.minimizeToTray);
        tray->update();
    }
} // namespace Soundux::Objects