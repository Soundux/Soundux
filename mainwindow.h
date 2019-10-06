#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <stdlib.h>
#include <regex>
#include <thread>
#include <filesystem>
#include <fstream>
#include <pthread.h>
#include <signal.h>

#include <QMainWindow>
#include <QFile>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QCloseEvent>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>

#include <json.hpp>

using namespace std;
using json = nlohmann::json;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct PulseAudioRecordingStream
{
        int index;
        string driver;
        string flags;
        string state;
        string source;
        bool muted;
        string applicationName;
        int processId;
        string processBinary;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);

private:
    bool isValidDevice(PulseAudioRecordingStream* stream);
    bool loadSources();
    void playSound(string path);
    void clearSoundFiles();
    void saveSoundFiles();
    void loadSoundFiles();
    string getCommandOutput(char cmd[]);

private slots:
    void on_refreshAppsButton_clicked();
    void on_playCustomButton_clicked();
    void on_customFileChoose_clicked();
    void on_stopButton_clicked();
    void on_addSoundButton_clicked();
    void on_removeSoundButton_clicked();
    void on_clearSoundsButton_clicked();
    void on_playSoundButton_clicked();

private:
    Ui::MainWindow *ui;
    std::thread forMe;
    std::thread forOthers;
};
#endif // MAINWINDOW_H