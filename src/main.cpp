#include <QQmlApplicationEngine>
#include <QApplication>
#include <QQuickStyle>
#include <QQmlContext>
#include <filesystem>
#include <fstream>
#include <qqml.h>
#include <qurl.h>

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

#ifdef _WIN32
INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
#else
int main(int argc, char **argv)
{
#endif

#ifdef _WIN32
    int argc;
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, nullptr);
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