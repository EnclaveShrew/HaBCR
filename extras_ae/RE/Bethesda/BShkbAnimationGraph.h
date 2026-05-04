#pragma once

#include "RE/Havok/hkbCharacter.h"

namespace RE
{
class BShkbAnimationGraph
{
  public:
    hkbCharacter &QCharacter()
    {
        return *reinterpret_cast<hkbCharacter *>(
            reinterpret_cast<std::uintptr_t>(this) + 0x1C8);
    }

    const hkbCharacter &QCharacter() const
    {
        return *reinterpret_cast<const hkbCharacter *>(
            reinterpret_cast<std::uintptr_t>(this) + 0x1C8);
    }
};
}
