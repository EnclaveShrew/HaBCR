#pragma once

#include "RE/H/hkReferencedObject.h"

namespace RE
{
class hkbBehaviorGraph;

class __declspec(novtable) hkbCharacter : public hkReferencedObject
{
  public:
    std::uint8_t pad10[0x70];
    hkbBehaviorGraph *behaviorGraph;
};
static_assert(offsetof(hkbCharacter, behaviorGraph) == 0x80);
}
