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

        webview =
            std::make_shared<Webview::Window>("Soundux", Soundux::Globals::gData.width, Soundux::Globals::gData.height);
        webview->setTitle("Soundux");
        webview->enableDevTools(std::getenv("SOUNDUX_DEBUG") != nullptr);    // NOLINT
        webview->enableContextMenu(std::getenv("SOUNDUX_DEBUG") != nullptr); // NOLINT

#ifdef _WIN32
        char rawPath[MAX_PATH];
        GetModuleFileNameA(nullptr, rawPath, MAX_PATH);

        auto path = std::filesystem::canonical(rawPath).parent_path() / "dist" / "index.html";
        tray = std::make_shared<Tray::Tray>("soundux-tray", IDI_ICON1);
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

        tray = std::make_shared<Tray::Tray>("soundux-tray", iconPath.u8string());
#endif

        webview->expose(Webview::Function("getSettings", []() { return Globals::gSettings; }));
        webview->expose(Webview::Function("isLinux", []() {
#if defined(__linux__)
            return true;
#else
            return false;
#endif
        }));
        webview->expose(Webview::Function("addTab", [this]() { return (addTab()); }));
        webview->expose(Webview::Function("getTabs", []() { return Globals::gData.getTabs(); }));
        webview->expose(Webview::Function("getPlayingSounds", []() { return Globals::gAudio.getPlayingSounds(); }));
        webview->expose(Webview::Function("playSound", [this](std::uint32_t id) { return playSound(id); }));
        webview->expose(Webview::Function("stopSound", [this](std::uint32_t id) { return stopSound(id); }));
        webview->expose(Webview::Function(
            "seekSound", [this](std::uint32_t id, std::uint64_t seekTo) { return seekSound(id, seekTo); }));
        webview->expose(Webview::Function("pauseSound", [this](std::uint32_t id) { return pauseSound(id); }));
        webview->expose(Webview::Function("resumeSound", [this](std::uint32_t id) { return resumeSound(id); }));
        webview->expose(Webview::Function("repeatSound",
                                          [this](std::uint32_t id, bool repeat) { return repeatSound(id, repeat); }));
        webview->expose(Webview::Function("stopSounds", [this]() { return stopSounds(); }));
        webview->expose(Webview::Function("changeSettings",
                                          [this](const Settings &newSettings) { return changeSettings(newSettings); }));
        webview->expose(Webview::Function("requestHotkey", [](bool state) {
            //
            Globals::gHotKeys.shouldNotify(state);
        }));
        webview->expose(Webview::Function(
            "setHotkey", [this](std::uint32_t id, const std::vector<int> &keys) { return setHotkey(id, keys); }));
        webview->expose(Webview::Function("getHotkeySequence",
                                          [this](const std::vector<int> &keys) { return getHotkeySequence(keys); }));
        webview->expose(Webview::Function("removeTab", [this](std::uint32_t id) { return removeTab(id); }));
        webview->expose(Webview::Function("refreshTab", [this](std::uint32_t id) { return refreshTab(id); }));
        webview->expose(Webview::Function(
            "moveTabs", [this](const std::vector<int> &newOrder) { return changeTabOrder(newOrder); }));
        webview->expose(Webview::Function(
            "markFavorite", [this](const std::uint32_t &id, bool favourite) { return markFavourite(id, favourite); }));
        webview->expose(Webview::Function("getFavorites", [this] { return getFavourites(); }));
        webview->expose(Webview::Function("isYoutubeDLAvailable", []() { return Globals::gYtdl.available(); }));
        webview->expose(
            Webview::AsyncFunction("getYoutubeDLInfo", [this](Webview::Promise promise, const std::string &url) {
                promise.resolve(Globals::gYtdl.getInfo(url));
            }));
        webview->expose(
            Webview::AsyncFunction("startYoutubeDLDownload", [this](Webview::Promise promise, const std::string &url) {
                promise.resolve(Globals::gYtdl.download(url));
            }));
        webview->expose(Webview::AsyncFunction("stopYoutubeDLDownload", [this](Webview::Promise promise) {
            std::thread killDownload([promise, this] {
                Globals::gYtdl.killDownload();
                promise.discard();
            });
            killDownload.detach();
        }));
        webview->expose(Webview::Function("getSystemInfo", []() -> std::string { return SystemInfo::getSummary(); }));
        webview->expose(Webview::AsyncFunction(
            "updateCheck", [this](Webview::Promise promise) { promise.resolve(VersionCheck::getStatus()); }));
        webview->expose(Webview::Function("isOnFavorites", [this](bool state) { isOnFavorites(state); }));
        webview->expose(Webview::Function("deleteSound", [this](std::uint32_t id) { return deleteSound(id); }));

#if !defined(__linux__)
        webview->expose(Webview::Function("getOutputs", [this]() { return getOutputs(); }));
#endif
#if defined(_WIN32)
        webview->expose(Webview::Function("openUrl", [](const std::string &url) {
            ShellExecuteA(nullptr, nullptr, url.c_str(), nullptr, nullptr, SW_SHOW);
        }));
#endif
#if defined(__linux__)
        webview->expose(Webview::Function("openUrl", [](const std::string &url) {
            if (system(("xdg-open \"" + url + "\"").c_str()) != 0) // NOLINT
            {
                Fancy::fancy.logTime().warning() << "Failed to open url " << url << std::endl;
            }
        }));
        webview->expose(Webview::Function("getOutputs", [this]() { return getOutputs(); }));
        webview->expose(Webview::Function("getPlayback", [this]() { return getPlayback(); }));
        webview->expose(
            Webview::Function("startPassthrough", [this](const std::string &app) { return startPassthrough(app); }));
        webview->expose(Webview::Function("stopPassthrough", [this]() { stopPassthrough(); }));
        webview->expose(
            Webview::Function("isSwitchOnConnectLoaded", []() { return Globals::gPulse.isSwitchOnConnectLoaded(); }));
        webview->expose(Webview::Function("unloadSwitchOnConnect", []() { Globals::gPulse.unloadSwitchOnConnect(); }));
#endif

        webview->setCloseCallback([this]() {
            tray->getEntries().at(1)->setText(translations.show);
            if (Globals::gSettings.minimizeToTray)
            {
                webview->hide();
                return true;
            }
            return false;
        });

        webview->setResizeCallback([](int width, int height) {
            Globals::gData.width = width;
            Globals::gData.height = height;
        });

#if defined(IS_EMBEDDED)
#if defined(__linux__)
        webview->setUrl("embedded://" + path.string());
#elif defined(_WIN32)
        webview->setUrl("file:///embedded/" + path.string());
#endif
#else
        webview->setUrl("file://" + path.string());
#endif

        webview->setNavigateCallback([this]([[maybe_unused]] const std::string &url) {
            auto future = std::make_shared<std::future<void>>();
            *future = std::async(std::launch::async, [future, this] {
                translations.settings = webview
                                            ->callFunction<std::string>(
                                                Webview::JavaScriptFunction("window.getTranslation", "settings.title"))
                                            .get();
                translations.tabHotkeys = webview
                                              ->callFunction<std::string>(Webview::JavaScriptFunction(
                                                  "window.getTranslation", "settings.tabHotkeysOnly"))
                                              .get();
                translations.muteDuringPlayback = webview
                                                      ->callFunction<std::string>(Webview::JavaScriptFunction(
                                                          "window.getTranslation", "settings.muteDuringPlayback"))
                                                      .get();
                translations.show =
                    webview
                        ->callFunction<std::string>(Webview::JavaScriptFunction("window.getTranslation", "tray.show"))
                        .get();
                translations.hide =
                    webview
                        ->callFunction<std::string>(Webview::JavaScriptFunction("window.getTranslation", "tray.hide"))
                        .get();
                translations.exit =
                    webview
                        ->callFunction<std::string>(Webview::JavaScriptFunction("window.getTranslation", "tray.exit"))
                        .get();

                tray->addEntry(Tray::Button(translations.exit, [this]() {
                    tray->exit();
                    webview->exit();
                }));
                tray->addEntry(Tray::Button(translations.hide, [this]() {
                    if (!webview->isHidden())
                    {
                        webview->hide();
                        tray->getEntries().at(1)->setText(translations.show);
                    }
                    else
                    {
                        webview->show();
                        tray->getEntries().at(1)->setText(translations.hide);
                    }
                }));

                auto settings = tray->addEntry(Tray::Submenu(translations.settings));
                settings->addEntries(
                    Tray::SyncedToggle(translations.muteDuringPlayback, Globals::gSettings.muteDuringPlayback,
                                       [this](bool state) {
                                           auto settings = Globals::gSettings;
                                           settings.muteDuringPlayback = state;
                                           changeSettings(settings);
                                       }),
                    Tray::SyncedToggle(translations.tabHotkeys, Globals::gSettings.tabHotkeysOnly, [this](bool state) {
                        auto settings = Globals::gSettings;
                        settings.tabHotkeysOnly = state;
                        changeSettings(settings);
                    }));
            });
        });
    }
    void WebView::mainLoop()
    {
        webview->run();
        if (tray)
        {
            tray->exit();
        }
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

        webview->runCode(js);
    }
    void WebView::onSoundFinished(const PlayingSound &sound)
    {
        Window::onSoundFinished(sound);
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.finishSound(JSON.parse(`" + soundObj + "`));";

        webview->runCode(js);
    }
    void WebView::onSoundPlayed(const PlayingSound &sound)
    {
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.onSoundPlayed(JSON.parse(`" + soundObj + "`));";

        webview->runCode(js);
    }
    void WebView::onSoundProgressed(const PlayingSound &sound)
    {
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.updateSound(JSON.parse(`" + soundObj + "`));";

        webview->runCode(js);
    }
    void WebView::onDownloadProgressed(float progress, const std::string &eta)
    {
        auto js = "window.downloadProgressed(" + std::to_string(progress) + ", `" + eta + "`);";
        webview->runCode(js);
    }
    void WebView::onError(const ErrorCode &error)
    {
        auto js = "window.onError(" + std::to_string(static_cast<std::uint8_t>(error)) + ");";
        webview->runCode(js);
    }
    void WebView::changeSettings(const Settings &newSettings)
    {
        Window::changeSettings(newSettings);
        tray->update();
    }
} // namespace Soundux::Objects