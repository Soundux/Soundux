#include "keys.hpp"

namespace Soundux::Objects
{
    bool Key::operator==(const Key &other) const
    {
        return other.key == key && other.type == type;
    }
} // namespace Soundux::Objects