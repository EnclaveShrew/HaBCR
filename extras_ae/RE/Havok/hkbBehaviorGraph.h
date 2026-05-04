#pragma once

#include "RE/H/hkArray.h"
#include "RE/Havok/hkbGenerator.h"

namespace RE
{
struct hkbNodeInfo;

class __declspec(novtable) hkbBehaviorGraph : public hkbGenerator
{
  public:
    hkArray<hkbNodeInfo *> *GetActiveNodes()
    {
        return *reinterpret_cast<hkArray<hkbNodeInfo *> **>(
            reinterpret_cast<std::uintptr_t>(this) + 0xE0);
    }
};
}
