#pragma once
#include <qlist.h>
#include <utility>
#include <vector>
#include <QString>
#include <QObject>
#include "../config/config.h"
#include "../hotkeys/global.h"

#ifdef __linux__
#include "../playback/linux.h"

struct QPulseAudioRecordingStream
{
    Q_GADGET
  public:
    void setInstance(Soundux::Playback::internal::PulseAudioRecordingStream instance)
    {
        this->instance = std::move(instance);
    }
    Q_INVOKABLE QString getName() const
    {
        return QString::fromStdString(instance.processBinary);
    }

  private:
    Soundux::Playback::internal::PulseAudioRecordingStream instance;
};

Q_DECLARE_METATYPE(QPulseAudioRecordingStream)
#endif

#ifdef _WIN32
struct QPulseAudioRecordingStream
{
};
#endif

struct QSound
{
    Q_GADGET
  public:
    void setInstance(Soundux::Config::Sound instance)
    {
        this->instance = std::move(instance);
    }
    Soundux::Config::Sound getInstance() const
    {
        return instance;
    }
    Q_INVOKABLE QString getName() const
    {
        return QString::fromStdString(instance.name);
    }
    Q_INVOKABLE QString getPath() const
    {
        return QString::fromStdString(instance.path);
    }
    Q_INVOKABLE QList<QString> getKeyBinds() const
    {
        auto hotKeys = instance.hotKeys;
        QList<QString> hotKeyStr;

        for (auto &key : hotKeys)
        {
            hotKeyStr.push_back(QString::fromStdString(Soundux::Hooks::getKeyName(key)));
        }

        return hotKeyStr;
    }

  private:
    Soundux::Config::Sound instance;
};

Q_DECLARE_METATYPE(QSound)

struct QTab
{
    Q_GADGET
  public:
    void setInstance(Soundux::Config::Tab instance)
    {
        this->instance = std::move(instance);
    }
    Soundux::Config::Tab getInstance() const
    {
        return instance;
    }
    Q_INVOKABLE QString getTitle() const
    {
        return QString::fromStdString(instance.title);
    }
    Q_INVOKABLE QString getFolder() const
    {
        return QString::fromStdString(instance.folder);
    }
    Q_INVOKABLE std::vector<QSound> getSounds() const
    {
        const auto &sounds = instance.sounds;

        std::vector<QSound> qSounds;
        for (const auto &sound : sounds)
        {
            QSound qSound;
            qSound.setInstance(sound);

            qSounds.push_back(qSound);
        }
        return qSounds;
    }

  private:
    Soundux::Config::Tab instance;
};

Q_DECLARE_METATYPE(QTab)