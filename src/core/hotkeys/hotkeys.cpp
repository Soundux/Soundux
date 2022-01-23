#include "hotkeys.hpp"
#include "keys.hpp"
#include "linux/x11.hpp"
#include "windows/windows.hpp"
#include <core/global/globals.hpp>
#include <cstdint>
#include <fancy.hpp>

namespace Soundux
{
    namespace traits
    {
        template <typename T> struct is_pair
        {
          private:
            static std::uint8_t test(...);
            template <typename O>
            static auto test(O *) -> decltype(std::declval<O>().first, std::declval<O>().second, std::uint16_t{});

          public:
            static const bool value = sizeof(test(reinterpret_cast<T *>(0))) == sizeof(std::uint16_t);
        };
    } // namespace traits
    namespace Objects
    {
        std::shared_ptr<Hotkeys> Hotkeys::createInstance()
        {
            std::shared_ptr<Hotkeys> rtn;
#if defined(__linux__)
            rtn = std::shared_ptr<X11>(new X11()); // NOLINT
#elif defined(_WIN32)
            rtn = std::shared_ptr<WindowsHotkeys>(new WindowsHotkeys()); // NOLINT
#endif
            rtn->setup();
            return rtn;
        }

        void Hotkeys::setup()
        {
            try
            {
                midi = std::make_unique<libremidi::midi_in>();

                midi->open_port();
                midi->set_callback([this](const libremidi::message &message) {
                    if (message.size() < 3)
                    {
                        Fancy::fancy.logTime().failure()
                            << "Midi Message contains less than 3 bytes, can't parse information";
                        return;
                    }

                    auto byte0 = message[0];
                    auto byte1 = message[1];
                    auto byte2 = message[2];

                    MidiKey key;
                    key.byte0 = byte0;
                    key.key = byte1;
                    key.byte2 = byte2;
                    key.type = Enums::KeyType::Midi;

                    if (byte0 == 144)
                    {
                        onKeyDown(key);
                    }
                    else if (byte0 == 128)
                    {
                        onKeyUp(key);
                    }
                    else if (byte0 == 176)
                    {
                        if (shouldNotifyKnob)
                        {
                            Globals::gGui->onHotKeyReceived({key}); // NOLINT
                        }
                        else
                        {
                            auto newVolume = static_cast<int>((static_cast<float>(byte2) / 127.f) * 100);

                            if (Globals::gSettings.localVolumeKnob && key == Globals::gSettings.localVolumeKnob)
                            {
                                Globals::gSettings.localVolume = newVolume;
                                Globals::gGui->onVolumeChanged();

                                Globals::gQueue.push([=]() { Globals::gGui->onLocalVolumeChanged(newVolume); });
                            }
                            else if (Globals::gSettings.remoteVolumeKnob && key == Globals::gSettings.remoteVolumeKnob)
                            {
                                Globals::gSettings.remoteVolume = newVolume;
                                Globals::gGui->onVolumeChanged();

                                Globals::gQueue.push([=]() { Globals::gGui->onRemoteVolumeChanged(newVolume); });
                            }
                        }
                    }
                });
                midi->ignore_types(false, false, false);
            }
            catch (const libremidi::midi_exception &e)
            {
                Fancy::fancy.logTime().failure() << "Failed to initialize libremidi: " << e.what() << std::endl;
            }
            catch (...)
            {
                Fancy::fancy.logTime().failure() << "Unknown libremidi exception" << std::endl;
            }
        }
        void Hotkeys::notify(bool state)
        {
            pressedKeys.clear();
            shouldNotify = state;
        }
        void Hotkeys::requestKnob(bool state)
        {
            shouldNotifyKnob = state;
        }
        void Hotkeys::onKeyUp(const Key &key)
        {
            if (std::find(pressedKeys.begin(), pressedKeys.end(), key) != pressedKeys.end())
            {
                if (shouldNotify)
                {
                    Globals::gGui->onHotKeyReceived(pressedKeys);
                    pressedKeys.clear();
                }
                else
                {
                    pressedKeys.erase(std::remove_if(pressedKeys.begin(), pressedKeys.end(),
                                                     [&](const auto &keyItem) { return key == keyItem; }),
                                      pressedKeys.end());
                }
            }
        }
        bool isCloseMatch(const std::vector<Key> &pressedKeys, const std::vector<Key> &keys)
        {
            if (pressedKeys.size() >= keys.size())
            {
                bool allMatched = true;
                for (const auto &key : keys)
                {
                    if (std::find(pressedKeys.begin(), pressedKeys.end(), key) == pressedKeys.end()) // NOLINT
                    {
                        allMatched = false;
                    }
                }
                return allMatched;
            }
            return false;
        }
        template <typename T> std::optional<Sound> getBestMatch(const T &list, const std::vector<Key> &pressedKeys)
        {
            std::optional<Sound> rtn;

            for (const auto &_sound : list)
            {
                const auto &sound = [&]() constexpr
                {
                    if constexpr (traits::is_pair<std::decay_t<decltype(_sound)>>::value)
                    {
                        return _sound.second.get();
                    }
                    else
                    {
                        return _sound;
                    }
                }
                ();

                if (sound.hotkeys.empty())
                    continue;

                if (pressedKeys == sound.hotkeys)
                {
                    rtn = sound;
                    break;
                }

                if (rtn && rtn->hotkeys.size() > sound.hotkeys.size())
                {
                    continue;
                }

                if (isCloseMatch(pressedKeys, sound.hotkeys))
                {
                    rtn = sound;
                }
            }
            return rtn;
        }
        void Hotkeys::onKeyDown(const Key &key)
        {
            if (std::find(pressedKeys.begin(), pressedKeys.end(), key) != pressedKeys.end())
            {
                return;
            }

            pressedKeys.emplace_back(key);
            if (!shouldNotify)
            {
                if (!Globals::gSettings.stopHotkey.empty() &&
                    (Globals::gSettings.stopHotkey == pressedKeys ||
                     isCloseMatch(pressedKeys, Globals::gSettings.stopHotkey)))
                {
                    Globals::gGui->stopSounds();
                    return;
                }

                std::optional<Sound> bestMatch;

                if (Globals::gSettings.tabHotkeysOnly)
                {
                    if (Globals::gData.isOnFavorites)
                    {
                        auto sounds = Globals::gData.getFavorites();
                        bestMatch = getBestMatch(sounds, pressedKeys);
                    }
                    else
                    {
                        auto tab = Globals::gData.getTab(Globals::gSettings.selectedTab);
                        if (tab)
                        {
                            bestMatch = getBestMatch(tab->sounds, pressedKeys);
                        }
                    }
                }
                else
                {
                    auto scopedSounds = Globals::gSounds.scoped();
                    bestMatch = getBestMatch(*scopedSounds, pressedKeys);
                }

                if (bestMatch)
                {
                    auto pSound = Globals::gGui->playSound(bestMatch->id);
                    if (pSound)
                    {
                        Globals::gGui->onSoundPlayed(*pSound);
                    }
                }
            }
        }
        std::string Hotkeys::getKeyName(const Key &key)
        {
            if (key.type == Enums::KeyType::Midi)
            {
                return "MIDI_" + std::to_string(key.key);
            }

            return "";
        }
        std::string Hotkeys::getKeySequence(const std::vector<Key> &keys)
        {
            std::string rtn;

            for (auto it = keys.begin(); it != keys.end(); ++it)
            {
                rtn += getKeyName(*it);
                if (std::distance(it, keys.end()) > 1)
                {
                    rtn += " + ";
                }
            }

            return rtn;
        }
    } // namespace Objects
} // namespace Soundux