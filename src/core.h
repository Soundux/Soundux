#pragma once
#include <QQmlApplicationEngine>
#include <QSharedPointer>
#include <qglobal.h>
#include <QObject>
#include <vector>

#include "config/config.h"
#include "bindings/bindings.h"
#include "qlist.h"

#ifdef _WIN32
#include "hotkeys/windows.h"
#include "playback/windows.h"
#else
#ifdef __linux__
#include "hotkeys/linux.h"
#include "playback/linux.h"
// #else
// #include "hotkeys/mac.h"
#endif
#endif

class Core : public QObject
{
    Q_OBJECT
  public:
    explicit Core(QObject * = 0);

  public slots:
    void setEngine(QQmlApplicationEngine *);
    void refresh();
    void onClose();

    void removeTab();
    QTab getCurrentTab();
    std::vector<QTab> getTabs();
    void addFolderTab(QList<QUrl>);
    void updateFolderSounds(QTab);
    void updateFolderSounds(Soundux::Config::Tab &);

    void changeLocalVolume(int);
    void changeRemoteVolume(int);

    std::vector<QSound> getAllSounds(std::string = "");
    std::vector<QSound> getSounds();
    void playSound(unsigned int);
    void playSound(std::string);
    void playSound(QString);
    void stopPlayback();

    void currentTabChanged(int);

    void setHotkey(int);
    void hotkeyDialogFocusChanged(int);

    int getDarkMode();
    void onDarkModeChanged(int);

    void onTabHotkeyOnlyChanged(int);
    int getTabHotkeysOnly();

    int isWindows();
    QList<QString> getCurrentHotKey(int);

    // Can't use preprocessor here because qt doesnt like that.

    /* Linux */
    void setLinuxSink(ma_device_info);
    void currentOutputApplicationChanged(int);
    std::vector<QPulseAudioRecordingStream> getOutputApplications();
    /* */

    /* Windows */
    QList<QString> getPlaybackDevices();
    /* */

  signals:
    void keyPress(QList<QString>);
    void keyCleared();

    void foldersChanged();
    void invalidApplication();

  private:
    QQmlApplicationEngine *engine{};
    ma_device_info sink;
};

inline Core gCore;