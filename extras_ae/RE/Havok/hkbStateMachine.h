#pragma once

#include "RE/Havok/hkbGenerator.h"
#include "RE/IDs_RTTI.h"
#include "RE/IDs_VTABLE.h"

namespace RE
{
class __declspec(novtable) hkbStateMachine : public hkbGenerator
{
  public:
    static constexpr auto RTTI{RTTI::hkbStateMachine};
    static constexpr auto VTABLE{VTABLE::hkbStateMachine};

    [[nodiscard]] std::int32_t GetCurrentStateId() const
    {
        return *reinterpret_cast<const std::int32_t *>(reinterpret_cast<std::uintptr_t>(this) + 0xC0);
    }
};
}
