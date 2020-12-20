#include <QQmlApplicationEngine>
#include <QApplication>
#include <QQuickStyle>
#include <QQmlContext>
#include <filesystem>
#include <fstream>
#include <qqml.h>
#include <qurl.h>
#include <string>

#include "core.h"
#include "config/config.h"
#include "playback/global.h"
#include "bindings/bindings.h"

#ifdef _WIN32
#include "hotkeys/windows.h"
#include "playback/windows.h"
#else
#ifdef __linux__
#include "hotkeys/linux.h"
#include "playback/linux.h"
#else
// #include "hotkeys/mac.h"
#endif
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    std::vector<char *> args(argv, argv + argc);

    // Thanks to this article https://kb.froglogic.com/squish/qt/howto/automating-native-file-dialogs/ !
    args.push_back(const_cast<char *>("-platformtheme"));
    args.push_back(const_cast<char *>("none"));

    argc = args.size();
    argv = args.data();

    QApplication app(argc, (char **)args.data());
#else
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
#endif
    QQmlApplicationEngine engine;
    QQuickStyle::setStyle("Material");

    app.setOrganizationDomain("https://github.com/D3SOX/Soundux");
    app.setOrganizationName("Soundux");

    Soundux::Config::loadConfig();
    Soundux::Playback::usedDevices = Soundux::Config::gConfig.volumes;

    gCore.setEngine(&engine);
    engine.rootContext()->setContextProperty("core", &gCore);

    // register meta types
    qRegisterMetaType<QTab>();
    qRegisterMetaType<std::vector<QTab>>();

    qRegisterMetaType<QSound>();
    qRegisterMetaType<std::vector<QSound>>();

#ifdef __linux__
    qRegisterMetaType<QPulseAudioRecordingStream>();
    qRegisterMetaType<std::vector<QPulseAudioRecordingStream>>();
#endif

    Soundux::Hooks::setup();

#ifdef __linux__
    Soundux::Playback::deleteSink();
    auto sinkName = Soundux::Playback::createSink();
    for (const auto &device : Soundux::Playback::getPlaybackDevices())
    {
        if (device.name == sinkName)
        {
            gCore.setLinuxSink(device);
            break;
        }
    }
#endif

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    return app.exec();
}