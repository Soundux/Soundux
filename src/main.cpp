#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <qqml.h>
#include <qurl.h>
#include <string>

#include "core.h"
#include "bindings/bindings.h"
#include "config/config.h"
#include "playback/global.h"

#ifdef _WIN32
#include "hotkeys/windows.h"
#include "playback/windows.h"
#else
#ifdef __linux__
#include "hotkeys/linux.h"
#include "playback/linux.h"
#include <csignal>
#include <execinfo.h>
#else
// #include "hotkeys/mac.h"
#endif
#endif

#ifdef __linux__
void sigHandler(int signal)
{
    std::cerr << "Received Signal: " << signal << std::endl;
    std::cerr << "Backtrace available" << std::endl;

    void *elements[20];
    auto size = backtrace(elements, 20);
    auto *stack = backtrace_symbols(elements, size);

    for (int i = 0; size > i; i++)
    {
        std::cerr << stack[i] << std::endl;
    }
    free(stack);

    exit(1);
}
#endif

void exceptionHandler()
{
    std::cerr << "An exception caused the program to crash" << std::endl;
    auto exception = std::current_exception();

    try
    {
        std::rethrow_exception(exception);
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        std::cerr << "Exception Type: " << typeid(e).name() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown exception" << std::endl;
    }

#ifdef __linux__
    std::cerr << "Backtrace available" << std::endl;
    void *elements[20];
    auto size = backtrace(elements, 20);
    auto *stack = backtrace_symbols(elements, size);

    for (int i = 0; size > i; i++)
    {
        std::cerr << stack[i] << std::endl;
    }
    free(stack);
#endif
}

int main(int argc, char **argv)
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#ifdef _WIN32
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    std::vector<char *> args(argv, argv + argc);

    // Thanks to this article https://kb.froglogic.com/squish/qt/howto/automating-native-file-dialogs/ !
    args.push_back(const_cast<char *>("-platformtheme"));
    args.push_back(const_cast<char *>("none"));

    argc = args.size();
    argv = args.data();

    QApplication app(argc, (char **)args.data());
#else
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
    std::set_terminate(exceptionHandler);

#ifdef __linux__
    signal(SIGSEGV, sigHandler);
    signal(SIGABRT, sigHandler);
    signal(SIGFPE, sigHandler);

    Soundux::Playback::deleteSink();
    Soundux::Playback::createSink();

    for (const auto &device : Soundux::Playback::getPlaybackDevices())
    {
        if (device.name == Soundux::Playback::internal::sinkName)
        {
            gCore.setLinuxSink(device);
            break;
        }
    }
#endif

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    static_cast<void>(app.exec());

    Soundux::Config::gConfig.volumes = Soundux::Playback::usedDevices;
    Soundux::Config::saveConfig();

    Soundux::Hooks::stop();

    Soundux::Playback::stopAllAudio();
#ifdef __linux__
    Soundux::Playback::deleteSink();
#endif
    Soundux::Playback::destroy();

    return 0;
}