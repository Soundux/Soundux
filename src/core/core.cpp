#include "core.h"
#include <filesystem>
#include <system_error>

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

void Core::refresh()
{
    this->engine->clearComponentCache();
    this->engine->load("../src/main.qml");
}

#ifdef __linux__
void Core::setLinuxSink(const ma_device_info &linuxSink)
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
void Core::setLinuxSink(const ma_device_info &linuxSink) {}
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

void Core::addFolderTab(const QUrl &folder)
{
    Soundux::Config::Tab tab;
    tab.title = folder.fileName().toStdString();
    tab.folder = folder.path().toStdString();

    updateFolderSounds(tab);

    Soundux::Config::gConfig.tabs.push_back(tab);

    Soundux::Config::saveConfig();
    emit foldersChanged();
}

void Core::updateFolderSounds(const QTab &qTab)
{
    // TODO(d3s0x): fix that the previously selected item is selected afterwards when it's still there
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

    if (std::filesystem::exists(path))
    {
        std::vector<Soundux::Config::Sound> newSounds;
        for (const auto &file : std::filesystem::directory_iterator(path))
        {
            if (file.path().extension() != ".mp3" && file.path().extension() != ".wav" &&
                file.path().extension() != ".flac")
            {
                continue;
            }

            Soundux::Config::Sound sound;

            std::error_code ec;
            auto writeTime = file.last_write_time(ec);
            if (!ec)
            {
                sound.lastWriteTime = writeTime.time_since_epoch().count();
            }
            else
            {
                std::cerr << "Failed to get last write time" << std::endl;
            }

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

        std::sort(newSounds.begin(), newSounds.end(),
                  [](auto &first, auto &second) { return first.lastWriteTime > second.lastWriteTime; });

        tab.sounds = newSounds;
    }
    else
    {
        Soundux::Config::gConfig.tabs.erase(
            std::remove(Soundux::Config::gConfig.tabs.begin(), Soundux::Config::gConfig.tabs.end(), tab));
        emit foldersChanged();
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

std::vector<QSound> Core::getAllSounds()
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

void Core::playSoundByPath(const QString &path)
{
    for (const auto &tab : Soundux::Config::gConfig.tabs)
    {
        auto sound = std::find_if(tab.sounds.begin(), tab.sounds.end(),
                                  [&](const auto &item) { return item.path == path.toStdString(); });
        if (sound != tab.sounds.end())
        {
            playSound(*sound);
            break;
        }
    }
}

void Core::playSound(int index)
{
    if (Soundux::Config::gConfig.currentTab < Soundux::Config::gConfig.tabs.size() &&
        Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds.size() > index)
    {
        playSound(Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab].sounds[index]);
    }
    else
    {
        std::cerr << "Invalid tab when playing audio" << std::endl;
    }
}

void Core::playSound(const Soundux::Config::Sound &sound)
{
    if (!Soundux::Config::gConfig.allowOverlapping)
    {
        Soundux::Playback::stopAllAudio();
    }
#ifdef __linux__
    static std::string moveBackCmd;

    auto outputApp = Soundux::Playback::getCurrentOutputApplication();

    if (outputApp)
    {
        emit playbackChanged(true);
        auto source = outputApp->source;

        if (source != Soundux::Playback::internal::sinkMonitorId)
        {
            auto moveToSink = "pactl move-source-output " + std::to_string(outputApp->index) + " " +
                              Soundux::Playback::internal::sinkMonitorId;
            moveBackCmd = "pactl move-source-output " + std::to_string(outputApp->index) + " " + source;

            static_cast<void>(system(moveToSink.c_str()));
        }

        // play on linux sink
        auto lastPlayedId = Soundux::Playback::playAudio(sound, sink);

        Soundux::Playback::stopCallback = [=](const auto &info) {
            if (info.id == lastPlayedId)
            {
                emit playbackChanged(false);
                static_cast<void>(system(moveBackCmd.c_str()));
            }
        };
    }
    else
    {
        emit playbackChanged(false);
        emit invalidApplication();
    }
#endif
#ifdef _WIN32
    Soundux::Playback::playAudio(sound, sink);
#endif
    // play for me on default playback device
    Soundux::Playback::playAudio(sound);
}

void Core::changeLocalVolume(int volume)
{
    Soundux::Playback::setVolume(Soundux::Playback::defaultPlayback.name, static_cast<float>(volume) / 100.F);
}

void Core::changeRemoteVolume(int volume)
{
    Soundux::Playback::setVolume(sink.name, static_cast<float>(volume) / 100.F);
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

void Core::setStopHotkey()
{
    Soundux::Hooks::internal::translateHotkeys = false;
    Soundux::Config::gConfig.stopHotKey = Soundux::Hooks::internal::capturedKeyList;
    Soundux::Config::saveConfig();
    Soundux::Hooks::internal::capturedKeyStates.clear();
}

void Core::setHotkey(const QString &sound)
{
    Soundux::Hooks::internal::translateHotkeys = false;

    auto &currentTab = Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab];
    auto item = std::find(currentTab.sounds.begin(), currentTab.sounds.end(), sound.toStdString());

    if (item != currentTab.sounds.end())
    {
        item->hotKeys = Soundux::Hooks::internal::capturedKeyList;
        emit updateCurrentTab();
    }
    else
    {
        std::cerr << "Cant find requested sound(" << sound.toStdString() << ")!" << std::endl;
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

QList<QString> Core::getStopHotKey()
{
    QList<QString> rtn;
    for (const auto &key : Soundux::Config::gConfig.stopHotKey)
    {
        rtn.push_back(QString::fromStdString(Soundux::Hooks::getKeyName(key)));
    }
    return rtn;
}
QList<QString> Core::getCurrentHotKey(const QString &sound)
{
    QList<QString> rtn;

    auto &currentTab = Soundux::Config::gConfig.tabs[Soundux::Config::gConfig.currentTab];
    auto item = std::find(currentTab.sounds.begin(), currentTab.sounds.end(), sound.toStdString());

    if (item != currentTab.sounds.end())
    {
        for (const auto &key : item->hotKeys)
        {
            rtn.push_back(QString::fromStdString(Soundux::Hooks::getKeyName(key)));
        }
    }
    else
    {
        std::cerr << "Cant find requested sound(" << sound.toStdString() << ")!" << std::endl;
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