#include <QQmlApplicationEngine>
#include <QApplication>
#include <QQuickStyle>
#include <QQmlContext>
#include <filesystem>
#include <fstream>
#include <qqml.h>

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
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);
    QQmlApplicationEngine engine;
    QQuickStyle::setStyle("Material");

    app.setOrganizationDomain("https://github.com/D3SOX/Soundux");
    app.setOrganizationName("Soundux");

    Soundux::Config::loadConfig();

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

    engine.load("qrc:/main.qml");
    return QGuiApplication::exec();
}