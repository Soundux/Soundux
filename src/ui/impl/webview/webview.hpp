#pragma once
#include "../../ui.hpp"
#include "lib/webview/webview.hpp"

namespace Soundux
{
    namespace Objects
    {
        class WebView : public Window
        {
          private:
            std::unique_ptr<wv::WebView> webview;

          public:
            void setup() override;
            void mainLoop() override;
            void onSoundFinished(const PlayingSound &sound) override;
            void onHotKeyReceived(const std::vector<int> &keys) override;

            void onError(const ErrorCode &error) override;
            void onSoundPlayed(const PlayingSound &sound) override;
            void onSoundProgressed(const PlayingSound &sound) override;
        };
    } // namespace Objects
} // namespace Soundux