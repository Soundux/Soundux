#include "core/global/globals.hpp"
#include "ui/impl/webview/webview.hpp"

int main()
{
#if defined(__linux__)
    if (!Soundux::Globals::gPulse.isSwitchOnConnectLoaded())
    {
        Soundux::Globals::gPulse.setup();
    }
    Soundux::Globals::gAudio.setup();
#endif
    Soundux::Globals::gConfig.load();
#if defined(__linux__)
    if (Soundux::Globals::gConfig.settings.useAsDefaultDevice)
    {
        Soundux::Globals::gPulse.setDefaultSourceToSoundboardSink();
    }
#endif
    Soundux::Globals::gData = Soundux::Globals::gConfig.data;
    Soundux::Globals::gSettings = Soundux::Globals::gConfig.settings;

    Soundux::Globals::gGui = std::make_unique<Soundux::Objects::WebView>();
    Soundux::Globals::gGui->setup();
    Soundux::Globals::gGui->mainLoop();

    Soundux::Globals::gAudio.destory();
    Soundux::Globals::gPulse.destroy();

    Soundux::Globals::gConfig.data = Soundux::Globals::gData;
    Soundux::Globals::gConfig.settings = Soundux::Globals::gSettings;
    Soundux::Globals::gConfig.save();

    return 0;
}