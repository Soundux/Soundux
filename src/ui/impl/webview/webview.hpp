#pragma once
#include <tray.hpp>
#include <ui/ui.hpp>
#include <webview.hpp>

namespace Soundux
{
    namespace Objects
    {
        class WebView : public Window
        {
          private:
            std::shared_ptr<Tray::Tray> tray;
            std::shared_ptr<Webview::Window> webview;
            void changeSettings(const Settings &newSettings) override;

            struct
            {
                std::string exit;
                std::string hide;
                std::string show;
                std::string settings;
                std::string tabHotkeys;
                std::string muteDuringPlayback;
            } translations;

          public:
            void setup() override;
            void mainLoop() override;
            void onHotKeyReceived(const std::vector<int> &keys) override;
            void onSoundFinished(const PlayingSound &sound, bool forced) override;

            void onError(const ErrorCode &error) override;
            void onSoundPlayed(const PlayingSound &sound) override;
            void onSoundProgressed(const PlayingSound &sound) override;
            void onDownloadProgressed(float progress, const std::string &eta) override;
        };
    } // namespace Objects
} // namespace Soundux