#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <array>
#include <stdlib.h>
#include <filesystem>
#include <fstream>


#include <QDialog>
#include <QCheckBox>
#include <QDialog>
#include <QListWidget>
#include <QTabWidget>
#include <QStackedWidget>
#include <QSplitter>

#include <settingstab.h>
#include <soundplayback.h>

class SoundPlayback;

using namespace std;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr, string configFolder = nullptr, SoundPlayback *soundPlayback = nullptr);

private slots:
    virtual void accept() override;

private:
    void AddTab(SettingsTab* tab, const QString& title);
    QList<SettingsTab*> tabs_;
    QTabWidget* tab_widget_;
};

#endif // SETTINGS_H
