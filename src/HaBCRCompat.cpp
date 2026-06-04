#include "HaBCRCompat.h"
#include "Hooks.h"

#include <limits>

namespace
{
constexpr std::uint32_t kModeNone = 0;
constexpr std::uint32_t kModeHaBCR = 1;
constexpr std::uint32_t kModeBCRCompatible = 2;

[[nodiscard]] bool IsValidAmmoCapacity(std::uint32_t a_capacity)
{
    return a_capacity <= std::numeric_limits<std::uint16_t>::max();
}

void ReapplyCurrentCapacity()
{
    auto &state = HaBCR::g_reloadState;
    if (state.IsReloading())
    {
        HaBCR::SetWeapAmmoCapacity(state.currentAmmoCount);
    }
}
} // namespace

extern "C" HABCR_COMPAT_API std::uint32_t HaBCR_GetCurrentReloadMode()
{
    auto &state = HaBCR::g_reloadState;
    if (!state.IsReloading())
    {
        return kModeNone;
    }

    switch (state.mode)
    {
    case HaBCR::ReloadMode::kHaBCR:
        return kModeHaBCR;
    case HaBCR::ReloadMode::kBCR:
        return kModeBCRCompatible;
    default:
        return kModeNone;
    }
}

extern "C" HABCR_COMPAT_API bool HaBCR_SetAmmoCapacity(std::uint32_t a_capacity)
{
    if (!IsValidAmmoCapacity(a_capacity))
    {
        return false;
    }

    auto &state = HaBCR::g_reloadState;
    if (!state.weapInstance)
    {
        return false;
    }

    state.ammoCapacity = static_cast<std::uint16_t>(a_capacity);
    ReapplyCurrentCapacity();
    return true;
}

extern "C" HABCR_COMPAT_API bool HaBCR_BeginAmmoSwitch()
{
    auto &state = HaBCR::g_reloadState;
    if (!state.IsReloading())
    {
        return false;
    }

    state.ammoSwitchInProgress = true;
    return true;
}

extern "C" HABCR_COMPAT_API bool HaBCR_EndAmmoSwitch()
{
    auto &state = HaBCR::g_reloadState;
    const bool wasInProgress = state.ammoSwitchInProgress;
    state.ammoSwitchInProgress = false;
    return wasInProgress;
}

extern "C" HABCR_COMPAT_API bool HaBCR_UpdateAmmoStateAfterSwitch(std::uint32_t a_loadedAmmoCount,
                                                                  std::uint32_t a_ammoCapacity,
                                                                  std::uint32_t a_totalAmmoCount)
{
    if (!IsValidAmmoCapacity(a_ammoCapacity))
    {
        return false;
    }

    auto &state = HaBCR::g_reloadState;
    if (!state.IsReloading() || !state.ammoSwitchInProgress)
    {
        return false;
    }

    state.currentAmmoCount = a_loadedAmmoCount;
    state.ammoCapacity = static_cast<std::uint16_t>(a_ammoCapacity);
    state.inventoryAmmoCount = a_totalAmmoCount > a_loadedAmmoCount ? a_totalAmmoCount - a_loadedAmmoCount : 0;
    state.startingAmmoCount = a_loadedAmmoCount >= state.countedRC ? a_loadedAmmoCount - state.countedRC : 0;

    if (state.mode == HaBCR::ReloadMode::kBCR)
    {
        state.waitingReloadEnd = state.IsLoopFinished();
    }

    ReapplyCurrentCapacity();
    return true;
}
