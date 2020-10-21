#include "core.h"

Core::Core(QObject *parent) : QObject(parent) {}

void Core::setEngine(QQmlApplicationEngine *engine)
{
    this->engine = engine;
}

void Core::onClose()
{
    Soundux::Hooks::stop();
}

void Core::refresh()
{
    this->engine->clearComponentCache();
    this->engine->load("../src/main.qml");
}