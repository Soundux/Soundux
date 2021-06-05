#pragma once

namespace Soundux
{
    namespace Enums
    {
        enum class KeyType
        {
            Keyboard,
            Mouse,
            Midi
        };
    } // namespace Enums

    namespace Objects
    {
        struct Key
        {
            int key;
            Enums::KeyType type;

            virtual ~Key() = default;
            bool operator==(const Key &) const;
        };
        struct MidiKey : public Key
        {
            int byte0; // Action
            int byte2; // Knob value / Press strength
            ~MidiKey() override = default;
        };
    } // namespace Objects
} // namespace Soundux