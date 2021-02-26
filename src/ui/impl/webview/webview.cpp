#include "webview.hpp"
#include "../../../core/global/globals.hpp"
#include "../../../helper/json/bindings.hpp"
#include <algorithm>
#include <cstdint>
#include <fancy.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <shellapi.h>
#include <windows.h>
#endif

namespace Soundux::Objects
{
    void WebView::setup()
    {
        Window::setup();
#ifdef _WIN32
        ::ShowWindow(::GetConsoleWindow(), SW_HIDE);

        char rawPath[MAX_PATH];
        auto executablePath = GetModuleFileNameA(nullptr, rawPath, MAX_PATH);

        auto path = std::filesystem::canonical(rawPath).parent_path() / "dist" / "index.html";
#endif
#if defined(__linux__)
        auto path = std::filesystem::canonical("/proc/self/exe").parent_path() / "dist" / "index.html";
#endif
        webview = std::make_unique<wv::WebView>(Globals::gData.width, Globals::gData.height, true, "Soundux",
                                                "file://" + path.string());
        if (!webview->init())
        {
            Fancy::fancy.logTime().failure() << "Failed to create UI" << std::endl;
        }

        webview->addCallback(
            "getPlayingSounds",
            [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                return nlohmann::json(Globals::gAudio.getPlayingSounds()).dump();
            },
            true);
        webview->addCallback(
            "getSettings",
            [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                return nlohmann::json(Globals::gSettings).dump();
            },
            true);
        webview->addCallback(
            "getData",
            [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                return nlohmann::json(Globals::gData).dump();
            },
            true);
        webview->addCallback(
            "addTab",
            [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                auto tab = addTab();
                if (tab)
                {
                    return nlohmann::json(*tab).dump();
                }
                return "false";
            },
            true);
        webview->addCallback(
            "playSound",
            [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
                auto sound = playSound(std::stoi(param[0]));
                if (sound)
                {
                    return nlohmann::json(*sound).dump();
                }
                return "false";
            },
            true);
        webview->addCallback("stopSound", [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
            if (stopSound(std::stoi(param[0])))
            {
                return "true";
            }
            return "false";
        });
        webview->addCallback("stopSounds",
                             [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                                 stopSounds();
                                 return "";
                             });
        webview->addCallback(
            "pauseSound",
            [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
                auto sound = pauseSound(std::stoi(param[0]));
                if (sound)
                {
                    return nlohmann::json(*sound).dump();
                }
                return "false";
            },
            true);
        webview->addCallback(
            "resumeSound",
            [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
                auto sound = resumeSound(std::stoi(param[0]));
                if (sound)
                {
                    return nlohmann::json(*sound).dump();
                }
                return "false";
            },
            true);
        webview->addCallback(
            "seekSound",
            [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
                auto sound = seekSound(std::stoi(param[0]), std::stoi(param[1]));
                if (sound)
                {
                    return nlohmann::json(*sound).dump();
                }
                return "false";
            },
            true);
        webview->addCallback("changeSettings", [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
            nlohmann::json j = nlohmann::json::parse(param[0]);
            changeSettings(j.get<Settings>());
            return "";
        });
        webview->addCallback("requestHotkey", [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
            if (param[0] == "false")
            {
                Globals::gHotKeys.shouldNotify(false);
            }
            else
            {
                Globals::gHotKeys.shouldNotify(true);
            }
            return "";
        });
        webview->addCallback(
            "setHotkey",
            [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
                auto sound = setHotkey(std::stoi(param[0]), nlohmann::json::parse("[" + param[1] + "]"));
                if (sound)
                {
                    return nlohmann::json(*sound).dump();
                }
                return "false";
            },
            true);
        webview->addCallback("getHotkeySequence", [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
            nlohmann::json j = nlohmann::json::parse("[" + param[0] + "]");
            return getHotkeySequence(j.get<std::vector<int>>());
        });
        webview->addCallback(
            "removeTab",
            [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
                return nlohmann::json(removeTab(std::stoi(param[0]))).dump();
            },
            true);
        webview->addCallback(
            "refreshTab",
            [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
                auto tab = refreshTab(std::stoi(param[0]));
                if (tab)
                {
                    return nlohmann::json(*tab).dump();
                }
                return "false";
            },
            true);
        webview->addCallback(
            "repeatSound",
            [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
                auto playingSound = repeatSound(std::stoi(param[0]), param[1] == "true");
                if (playingSound)
                {
                    return nlohmann::json(*playingSound).dump();
                }
                return "false";
            },
            true);
        webview->addCallback(
            "isLinux",
            [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
#if defined(__linux__)
                return "true";
#else
                return "false";
#endif
            },
            true);

#if !defined(__linux__)
        webview->addCallback(
            "getOutput",
            [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                return nlohmann::json(getOutput()).dump();
            },
            true);
#if defined(_WIN32)
        webview->addCallback("openUrl", [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
            ShellExecuteA(0, 0, param[0].c_str(), 0, 0, SW_SHOW);
            return "";
        });
#else
// TODO(curve): Mac
#endif
#else
        webview->addCallback("openUrl", [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
            system(("xdg-open \"" + param[0] + "\"").c_str());
            return "";
        });
        webview->addCallback(
            "getOutput",
            [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                return nlohmann::json(getOutput()).dump();
            },
            true);
        webview->addCallback(
            "getPlayback",
            [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                return nlohmann::json(getPlayback()).dump();
            },
            true);
        webview->addCallback(
            "startPassthrough",
            [this]([[maybe_unused]] auto &wv, const auto &param) -> std::string {
                auto playback = startPassthrough(param[0]);
                if (playback)
                {
                    return nlohmann::json(*playback).dump();
                }
                return "false";
            },
            true);
        webview->addCallback("stopPassthrough",
                             [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                                 stopPassthrough();
                                 return "";
                             });
        webview->addCallback(
            "isSwitchOnConnectLoaded",
            [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                return Globals::gPulse.isSwitchOnConnectLoaded() ? "true" : "false";
            },
            true);
        webview->addCallback("unloadSwitchOnConnect",
                             [this]([[maybe_unused]] auto &wv, [[maybe_unused]] const auto &param) -> std::string {
                                 if (Globals::gPulse.isSwitchOnConnectLoaded())
                                 {
                                     Globals::gPulse.unloadSwitchOnConnect();
                                     Globals::gPulse.setup();
                                 }
                                 return "";
                             });
#endif

        webview->setResizeCallback([this](int width, int height) {
            Globals::gData.width = width;
            Globals::gData.height = height;
        });
    }
    void WebView::mainLoop()
    {
        while (webview->run())
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

        onEvent([js, this]() { webview->eval(js); });
    }
    void WebView::onSoundFinished(const PlayingSound &sound)
    {
        Window::onSoundFinished(sound);
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.finishSound(JSON.parse(`" + soundObj + "`));";

        onEvent([js, this]() { webview->eval(js); });
    }
    void WebView::onSoundPlayed(const PlayingSound &sound)
    {
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.onSoundPlayed(JSON.parse(`" + soundObj + "`));";

        onEvent([js, this]() { webview->eval(js); });
    }
    void WebView::onSoundProgressed(const PlayingSound &sound)
    {
        auto soundObj = nlohmann::json(sound).dump();
        auto js = "window.updateSound(JSON.parse(`" + soundObj + "`));";

        onEvent([js, this]() { webview->eval(js); });
    }
    void WebView::onError(const ErrorCode &error)
    {
        auto js = "window.onError(" + std::to_string(static_cast<std::uint8_t>(error)) + ");";
        onEvent([js, this]() { webview->eval(js); });
    }
} // namespace Soundux::Objects