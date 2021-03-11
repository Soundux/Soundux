#include "webview.hpp"
#include "../../../core/global/globals.hpp"
#include "../../../helper/json/bindings.hpp"
#include <algorithm>
#include <cstdint>
#include <fancy.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <optional>

#ifdef _WIN32
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
#endif
#if defined(__linux__)
        auto path = std::filesystem::canonical("/proc/self/exe").parent_path() / "dist" / "index.html";
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

#if !defined(__linux__)
        webview.addCallback("getOutputs", [this]() { return getOutputs(); });
#endif
#if defined(_WIN32)
        webview.addCallback("openUrl", [](const std::string &url) {
            ShellExecuteA(nullptr, nullptr, url.c_str(), nullptr, nullptr, SW_SHOW);
        });
#endif
#if defined(__linux__)
        webview.addCallback("openUrl", [this](const std::string &url) {
            if (system(("xdg-open \"" + url + "\"").c_str()) != 0) // NOLINT
            {
                Fancy::fancy.logTime().warning() << "Failed to open url " << url << std::endl;
            }
        });
        webview.addCallback("getOutputs", [this]() { return getOutputs(); });
        webview.addCallback("getPlayback", [this]() { return getPlayback(); });
        webview.addCallback("startPassthrough", [this](const std::string &app) { return startPassthrough(app); });
        webview.addCallback("stopPassthrough", [this]() { stopPassthrough(); });
        webview.addCallback("isSwitchOnConnectLoaded", [this]() { return Globals::gPulse.isSwitchOnConnectLoaded(); });
        webview.addCallback("unloadSwitchOnConnect", [this]() { Globals::gPulse.unloadSwitchOnConnect(); });
#endif

        webview.setResizeCallback([](int width, int height) {
            Globals::gData.width = width;
            Globals::gData.height = height;
        });

        webview.navigate("file://" + path.string());
    }
    void WebView::mainLoop()
    {
        while (webview.run())
        {
            progressEvents();
#if defined(_WIN32)
            Sleep(20);
#endif
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

        onEvent([js, this]() { webview.runCode(js); });
    }
    void WebView::onSoundFinished(const PlayingSound &sound)
    {
        Window::onSoundFinished(sound);
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.finishSound(JSON.parse(`" + soundObj + "`));";

        onEvent([js, this]() { webview.runCode(js); });
    }
    void WebView::onSoundPlayed(const PlayingSound &sound)
    {
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.onSoundPlayed(JSON.parse(`" + soundObj + "`));";

        onEvent([js, this]() { webview.runCode(js); });
    }
    void WebView::onSoundProgressed(const PlayingSound &sound)
    {
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.updateSound(JSON.parse(`" + soundObj + "`));";

        onEvent([js, this]() { webview.runCode(js); });
    }
    void WebView::onError(const ErrorCode &error)
    {
        auto js = "window.onError(" + std::to_string(static_cast<std::uint8_t>(error)) + ");";
        onEvent([js, this]() { webview.runCode(js); });
    }
} // namespace Soundux::Objects