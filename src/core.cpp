#include "core.h"
#include "bindings/bindings.h"
#include "config/config.h"
#include "hotkeys/global.h"
#include "playback/global.h"
#include "playback/linux.h"
#include <filesystem>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

Core::Core(QObject *parent) : QObject(parent) {}

void Core::setEngine(QQmlApplicationEngine *engine)
{
    this->engine = engine;
}

void Core::loadSettings()
{
    if (Soundux::Playback::usedDevices.find(Soundux::Playback::defaultPlayback.name) !=
        Soundux::Playback::usedDevices.end())
    {
        setLocalVolume(Soundux::Playback::usedDevices[Soundux::Playback::defaultPlayback.name]);
    }
#ifdef __linux__
    if (Soundux::Playback::usedDevices.find(Soundux::Playback::internal::sinkName) !=
        Soundux::Playback::usedDevices.end())
    {
        setRemoteVolume(Soundux::Playback::usedDevices[Soundux::Playback::internal::sinkName]);
    }

    auto index = Soundux::Config::gConfig.currentOutputApplication;
    auto sources = Soundux::Playback::getSources();
    if (sources.size() > index)
    {
        setOutputApplication(index);
    }
#endif
#ifdef _WIN32
    auto index = Soundux::Config::gConfig.currentOutputApplication;
    auto devices = Soundux::Playback::getPlaybackDevices();
    if (devices.size() > index)
    {
        auto &device = devices[index];
        if (Soundux::Playback::usedDevices.find(device.name) != Soundux::Playback::usedDevices.end())
        {
            setRemoteVolume(Soundux::Playback::usedDevices[device.name]);
        }
    }
#endif
    setSize(Soundux::Config::gConfig.width, Soundux::Config::gConfig.height);
}

void Core::onClose()
{
    Soundux::Config::gConfig.volumes = Soundux::Playback::usedDevices;
    Soundux::Config::saveConfig();

    Soundux::Hooks::stop();

    Soundux::Playback::stopAllAudio();
#ifdef __linux__
    Soundux::Playback::deleteSink();
#endif
    Soundux::Playback::destroy();
}

void Core::refresh()
{
    this->engine->clearComponentCache();
    this->engine->load("../src/main.qml");
}

#ifdef __linux__
void Core::setLinuxSink(ma_device_info linuxSink)
{
    sink = linuxSink;
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
void Core::currentOutputApplicationChanged(int index)
{
    if (index >= 0)
    {
        Soundux::Config::gConfig.currentOutputApplication = index;
    }
}
// Windows function that we need to define for moc
QList<QString> Core::getPlaybackDevices()
{
    return {};
}
#else
#ifdef _WIN32
// Define Linux functions to prevent moc failure.
void Core::setLinuxSink(ma_device_info linuxSink) {}
std::vector<QPulseAudioRecordingStream> Core::getOutputApplications()
{
    return {};
}

// Windows functions
void Core::currentOutputApplicationChanged(int index)
{
    Soundux::Config::gConfig.currentOutputApplication = index;
    auto devices = Soundux::Playback::getPlaybackDevices();
    if (devices.size() > (unsigned int)index)
    {
        sink = devices[index];
    }
}
QList<QString> Core::getPlaybackDevices()
{
    auto devices = Soundux::Playback::getPlaybackDevices();
    QList<QString> rtn;
    for (auto &device : devices)
    {
        rtn.push_back(QString::fromStdString(device.name));
    }
    return rtn;
}
#endif
#endif

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
    // TODO: fix that the previously selected item is selected afterwards when it's still there
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
#ifdef _WIN32
    // Please dont ask why we have to do this. I don't know either...
    auto path = tab.folder;
    if (path[0] == '/')
    {
        path = path.substr(1);
    }
#else
    auto path = tab.folder;
#endif

    std::vector<Soundux::Config::Sound> newSounds;
    for (const auto &file : std::filesystem::directory_iterator(path))
    {
        if (file.path().extension() != ".mp3" && file.path().extension() != ".wav")
            continue;

        Soundux::Config::Sound sound;
        sound.name = file.path().filename().u8string();
        sound.path = file.path().u8string();

        if (auto oldSound = std::find_if(tab.sounds.begin(), tab.sounds.end(),
                                         [&](auto &item) { return item.path == file.path().u8string(); });
            oldSound != tab.sounds.end())
        {
            sound.hotKeys = oldSound->hotKeys;
        }

        newSounds.push_back(sound);
    }

    tab.sounds = newSounds;
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
    if (!Soundux::Config::gConfig.allowOverlapping)
    {
        Soundux::Playback::stopAllAudio();
    }
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
        auto lastPlayedId = Soundux::Playback::playAudio(path, sink);

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
    Soundux::Playback::playAudio(path, sink);
#endif
    // play for me on default playback device
    Soundux::Playback::playAudio(path);
}

void Core::changeLocalVolume(int volume)
{
    Soundux::Playback::setVolume(Soundux::Playback::defaultPlayback.name, volume / 100.f);
}

void Core::changeRemoteVolume(int volume)
{
    Soundux::Playback::setVolume(sink.name, volume / 100.f);
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
    }
    else if (Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds.size() > (unsigned int)index)
    {
        Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds[index].hotKeys =
            Soundux::Hooks::internal::capturedKeyList;
        emit updateCurrentTab();
    }
    Soundux::Config::saveConfig();
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

int Core::isWindows()
{
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

void Core::onSizeChanged(int width, int height)
{
    Soundux::Config::gConfig.width = width;
    Soundux::Config::gConfig.height = height;
}

void Core::onAllowOverlappingChanged(int state)
{
    Soundux::Config::gConfig.allowOverlapping = state;
    Soundux::Config::saveConfig();
}

int Core::getAllowOverlapping()
{
    return Soundux::Config::gConfig.allowOverlapping;
}