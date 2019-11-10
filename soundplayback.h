#ifndef SOUNDPLAYBACK_H
#define SOUNDPLAYBACK_H

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
#include <algorithm>


#include <QObject>
#include <QMainWindow>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QTreeView>
#include <QListWidgetItem>
#include <QCloseEvent>
#include <QMessageBox>
#include <QStandardPaths>
#include <QInputDialog>
#include <QLineEdit>
#include <QTimer>

#include <ui_mainwindow.h>
#include <mainwindow.h>


using namespace std;

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

struct PulseAudioPlaybackStream
{
    int index;
    string applicationName;
};

class SoundPlayback : public QObject
{
    Q_OBJECT

public:
    explicit SoundPlayback(QWidget *parent = nullptr, Ui::MainWindow *mainWindow = nullptr);
    ~SoundPlayback();
    string getCommandOutput(char cmd[]);
    bool isValidDevice(PulseAudioRecordingStream *stream);
    void playSound(string path);
    void stopSound();
    bool loadSources();
    void checkAndChangeVolume(PulseAudioPlaybackStream *stream, int value);

    QWidget *parent;
    Ui::MainWindow* ui;
};

#endif // SOUNDPLAYBACK_H
