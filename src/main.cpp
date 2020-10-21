#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include "core.h"
#ifdef _WIN32
#include "hotkeys/windows.h"
#else
#ifdef __linux__
#include "hotkeys/linux.h"
// #else
// #include "hotkeys/mac.h"
#endif
#endif

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    QQuickStyle::setStyle("Material");

    Core core;
    core.setEngine(&engine);
    engine.rootContext()->setContextProperty("core", &core);

    Soundux::Hooks::setup();

    engine.load("qrc:/main.qml");
    return QGuiApplication::exec();
}