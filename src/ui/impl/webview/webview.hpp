#pragma once
#include "../../ui.hpp"
#include <tray.hpp>
#include <webview.hpp>

namespace Soundux
{
    namespace Objects
    {
        class WebView : public Window
        {
          private:
            SounduxWebView webview;
            std::shared_ptr<Tray> tray;
            void changeSettings(const Settings &newSettings) override;

          public:
            void setup() override;
            void mainLoop() override;
            void onSoundFinished(const PlayingSound &sound) override;
            void onHotKeyReceived(const std::vector<int> &keys) override;

            void onError(const ErrorCode &error) override;
            void onSoundPlayed(const PlayingSound &sound) override;
            void onSoundProgressed(const PlayingSound &sound) override;
            void onDownloadProgressed(float progress, const std::string &eta) override;
        };
    } // namespace Objects
} // namespace Soundux