#pragma once
#include <QObject>
#include <QQmlApplicationEngine>

#ifdef _WIN32
#include "hotkeys/windows.h"
#include "playback/windows.h"
#else
#ifdef __linux__
#include "hotkeys/linux.h"
// #else
// #include "hotkeys/mac.h"
#endif
#endif

class Core : public QObject
{
    Q_OBJECT
  public:
    explicit Core(QObject *parent = 0);

  public slots:
    void setEngine(QQmlApplicationEngine *engine);
    void refresh();
    void onClose();

  private:
    QQmlApplicationEngine *engine{};
};