#pragma once

#include "REL/Offset.h"
#include "REL/Relocation.h"
#include "RE/B/BSTEvent.h"
#include "RE/T/TESEquipEvent.h"

namespace RE
{
inline BSTEventSource<TESEquipEvent> &GetTESEquipEventSource()
{
    using func_t = BSTEventSource<TESEquipEvent> &(*)();
    static REL::Relocation<func_t> func{REL::Offset(0x531570)};
    return func();
}
}
