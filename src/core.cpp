#include "core.h"
#include "bindings/bindings.h"
#include "config/config.h"
#include "hotkeys/global.h"
#include "playback/global.h"
#include "playback/linux.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

Core::Core(QObject *parent) : QObject(parent) {}

void Core::setEngine(QQmlApplicationEngine *engine)
{
    this->engine = engine;
}

void Core::onClose()
{
    Soundux::Hooks::stop();

    Soundux::Playback::stopAllAudio();
    Soundux::Playback::deleteSink();
    Soundux::Playback::destroy();
}

void Core::refresh()
{
    this->engine->clearComponentCache();
    this->engine->load("../src/main.qml");
}

void Core::setLinuxSink(ma_device_info linuxSink)
{
    this->linuxSink = linuxSink;
}

std::vector<QPulseAudioRecordingStream> Core::getOutputApplications()
{
    std::vector<QPulseAudioRecordingStream> qStreams;
    auto pulseStreams = Soundux::Playback::getSources();
    for (const auto &pulseStream : pulseStreams)
    {
        QPulseAudioRecordingStream stream;
        stream.setInstance(pulseStream);

        qStreams.push_back(stream);
    }

    return qStreams;
}

std::vector<QTab> Core::getTabs()
{
    std::vector<QTab> qTabs;
    for (const auto &tab : Soundux::Config::gConfig.tabs)
    {
        QTab qTab;
        qTab.setInstance(tab);

        qTabs.push_back(qTab);
    }

    return qTabs;
}

void Core::addTab(QString title)
{
    Soundux::Config::Tab tab;
    tab.title = title.toStdString();
    Soundux::Config::gConfig.tabs.push_back(tab);
    Soundux::Config::saveConfig();
    emit foldersChanged();
}

void Core::addFolderTab(QList<QUrl> folders)
{
    for (QUrl folder : folders)
    {
        Soundux::Config::Tab tab;
        tab.title = folder.fileName().toStdString();
        tab.folder = folder.path().toStdString();

        updateFolderSounds(tab);

        Soundux::Config::gConfig.tabs.push_back(tab);
    }

    Soundux::Config::saveConfig();
    emit foldersChanged();
}

void Core::updateFolderSounds(QTab qTab)
{
    auto instance = qTab.getInstance();
    for (auto &tab : Soundux::Config::gConfig.tabs)
    {
        if (instance == tab)
        {
            updateFolderSounds(tab);
            break;
        }
    }

    Soundux::Config::saveConfig();
}

void Core::updateFolderSounds(Soundux::Config::Tab &tab)
{
    tab.sounds.clear();
    for (const auto &file : std::filesystem::directory_iterator(tab.folder))
    {
        if (file.path().extension() != ".mp3")
            continue;

        Soundux::Config::Sound sound;
        sound.name = file.path().filename();
        sound.path = file.path();
        tab.sounds.push_back(sound);
    }
}

void Core::removeTab()
{
    auto index = Soundux::Config::gConfig.currentTab;

    if (Soundux::Config::gConfig.tabs.size() > index)
    {

        Soundux::Config::gConfig.tabs.erase(Soundux::Config::gConfig.tabs.begin() + index);
        Soundux::Config::saveConfig();
        emit foldersChanged();
    }
}

void Core::currentTabChanged(int index)
{
    if (index >= 0)
    {
        Soundux::Config::gConfig.currentTab = index;
    }
}

std::vector<QSound> Core::getSounds()
{
    std::vector<QSound> qSounds;
    if (Soundux::Config::gConfig.tabs.size() > Soundux::Config::gConfig.currentTab)
    {
        for (const auto &sound : Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds)
        {
            QSound qSound;
            qSound.setInstance(sound);

            qSounds.push_back(qSound);
        }
    }
    else
    {
        Soundux::Config::gConfig.currentTab = Soundux::Config::gConfig.tabs.size() - 1;
    }
    return qSounds;
}

std::vector<QSound> Core::getAllSounds(std::string name)
{
    std::vector<QSound> qSounds;
    for (auto &tab : Soundux::Config::gConfig.tabs)
    {
        for (const auto &sound : tab.sounds)
        {
            QSound qSound;
            qSound.setInstance(sound);

            qSounds.push_back(qSound);
        }
    }
    return qSounds;
}

void Core::currentOutputApplicationChanged(int index)
{
    if (index >= 0)
    {
        Soundux::Config::gConfig.currentOutputApplication = index;
    }
}

void Core::playSound(unsigned int index)
{
    if (Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds.size() > index)
        playSound(Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds[index].path);
}

void Core::playSound(QString path)
{
    playSound(path.toStdString());
}

void Core::playSound(std::string path)
{
#ifdef __linux__
    static std::string moveBackCmd;
    static const std::string sinkMonitor = Soundux::Playback::internal::sinkName + ".monitor";

    auto outputApplications = Soundux::Playback::getSources();
    auto outputApp = Soundux::Playback::getCurrentOutputApplication();

    if (outputApp)
    {
        auto source = outputApp->source;

        if (source != sinkMonitor)
        {
            auto moveToSink = "pacmd move-source-output " + std::to_string(outputApp->index) + " " + sinkMonitor;
            moveBackCmd = "pacmd move-source-output " + std::to_string(outputApp->index) + " " + source;

            system(moveToSink.c_str());
        }

        // play on linux sink
        auto lastPlayedId = Soundux::Playback::playAudio(path, linuxSink);

        Soundux::Playback::stopCallback = [=](const auto &info) {
            if (info.id == lastPlayedId)
            {
                system(moveBackCmd.c_str());
            }
        };
    }
    else
    {
        emit invalidApplication();
    }
#endif
#ifdef _WIN32
// TODO: play on windows sink
#endif
    // play for me on default playback device
    Soundux::Playback::playAudio(path);
}

void Core::changeLocalVolume(int volume)
{
    Soundux::Playback::setVolume(Soundux::Playback::getDefaultPlaybackDevice().name, volume / 100.f);
}

void Core::changeRemoteVolume(int volume)
{
    Soundux::Playback::setVolume(linuxSink.name, volume / 100.f);
}

void Core::stopPlayback()
{
    Soundux::Playback::stopAllAudio();
}

QTab Core::getCurrentTab()
{
    QTab qTab;
    if (Soundux::Config::gConfig.tabs.size() > Soundux::Config::gConfig.currentTab)
    {
        qTab.setInstance(Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab]);
    }
    return qTab;
}

void Core::hotkeyDialogFocusChanged(int focus)
{
    if (focus)
    {
        Soundux::Hooks::internal::translateHotkeys = true;
        Soundux::Hooks::internal::capturedKeyList.clear();
        Soundux::Hooks::internal::capturedKeyStates.clear();
    }
    else
    {
        Soundux::Hooks::internal::translateHotkeys = false;
        Soundux::Hooks::internal::capturedKeyStates.clear();
    }
}

void Core::setHotkey(int index)
{
    Soundux::Hooks::internal::translateHotkeys = false;
    if (index == -100)
    {
        Soundux::Config::gConfig.stopHotKey = Soundux::Hooks::internal::capturedKeyList;
        Soundux::Config::saveConfig();
    }
    else if (Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds.size() > (unsigned int)index)
    {
        Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds[index].hotKeys =
            Soundux::Hooks::internal::capturedKeyList;
        Soundux::Config::saveConfig();
    }
    Soundux::Hooks::internal::capturedKeyStates.clear();
}

int Core::getDarkMode()
{
    return Soundux::Config::gConfig.darkTheme;
}

void Core::onDarkModeChanged(int mode)
{
    Soundux::Config::gConfig.darkTheme = mode;
    Soundux::Config::saveConfig();
}

void Core::onTabHotkeyOnlyChanged(int state)
{
    Soundux::Config::gConfig.tabHotkeysOnly = state;
    Soundux::Config::saveConfig();
}
int Core::getTabHotkeysOnly()
{
    return Soundux::Config::gConfig.tabHotkeysOnly;
}

QList<QString> Core::getCurrentHotKey(int index)
{
    QList<QString> rtn;
    if (index == -100)
    {
        for (const auto &key : Soundux::Config::gConfig.stopHotKey)
        {
            rtn.push_back(QString::fromStdString(Soundux::Hooks::getKeyName(key)));
        }
        Soundux::Hooks::internal::capturedKeyList = Soundux::Config::gConfig.stopHotKey;
    }
    else if (Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds.size() > (unsigned int)index)
    {
        for (const auto &key : Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds[index].hotKeys)
        {
            rtn.push_back(QString::fromStdString(Soundux::Hooks::getKeyName(key)));
        }
        Soundux::Hooks::internal::capturedKeyList =
            Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds[index].hotKeys;
    }
    return rtn;
}