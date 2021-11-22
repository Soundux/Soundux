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

            bool onClose();
            void exposeFunctions();
            void onResize(int, int);

            void setupTray();
            void fetchTranslations();

            void onAllSoundsFinished() override;
            Settings changeSettings(Settings newSettings) override;

          public:
            void show() override;
            void setup() override;
            void mainLoop() override;
            void onHotKeyReceived(const std::vector<Key> &keys) override;

            void onAdminRequired() override;
            void onSettingsChanged() override;
            void onLocalVolumeChanged(int volume) override;
            void onRemoteVolumeChanged(int volume) override;
            void onError(const Enums::ErrorCode &error) override;
            void onDownloadProgressed(float progress, const std::string &eta) override;

            void onSoundPlayed(const std::shared_ptr<PlayingSound> &sound) override;
            void onSoundFinished(const std::shared_ptr<PlayingSound> &sound) override;
            void onSoundProgressed(const std::shared_ptr<PlayingSound> &sound) override;
        };
    } // namespace Objects
} // namespace Soundux