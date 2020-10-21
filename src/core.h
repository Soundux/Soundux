#pragma once
#include <QObject>
#include <QQmlApplicationEngine>
#include <string>

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