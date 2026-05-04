#pragma once

namespace HaBCR
{
enum class ReloadPhase : std::uint8_t
{
    kIdle,
    kStarted,
    kLooping,
};

enum class ReloadMode : std::uint8_t
{
    kNone,
    kHaBCR,
    kBCR,
};

struct ReloadState
{
    RE::TESObjectWEAP::InstanceData *weapInstance{nullptr};
    RE::hkbClipGenerator *activeClipGenerator{nullptr};

    std::uint32_t startingAmmoCount{0};
    std::uint32_t currentAmmoCount{0};
    std::uint32_t countedRC{0};
    std::uint32_t loopCount{0};
    std::uint32_t inventoryAmmoCount{0};
    std::uint32_t RCPerLoop{1};
    std::uint32_t currentLoopRCCount{0};
    std::uint32_t nextCancelPointIndex{0};
    std::uint32_t beforeLoopRCCount{0};
    std::uint32_t afterLoopRCCount{0};

    std::vector<float> cancelPointTimes;

    float loopStartTime{0.0f};
    float loopEndTime{0.0f};
    float outroStartTime{0.0f};

    std::uint16_t ammoCapacity{0};

    ReloadPhase phase{ReloadPhase::kIdle};
    ReloadMode mode{ReloadMode::kNone};
    bool introEssential{false};
    bool stopPressed{false};
    bool waitingReloadEnd{false};

    std::uint16_t deferredCapacityRestore{0};

    void Reset();

    [[nodiscard]] bool IsReloading() const
    {
        return phase != ReloadPhase::kIdle;
    }

    [[nodiscard]] bool IsTacticalReloading() const
    {
        return startingAmmoCount > 0;
    }

    [[nodiscard]] std::uint32_t ShellsToAdd() const
    {
        auto fromCapacity = ammoCapacity > startingAmmoCount ? ammoCapacity - startingAmmoCount : 0;
        return fromCapacity < inventoryAmmoCount ? fromCapacity : inventoryAmmoCount;
    }

    [[nodiscard]] bool IsLoopFinished() const
    {
        return countedRC + afterLoopRCCount >= ShellsToAdd();
    }

    [[nodiscard]] float CompletionJumpTarget() const
    {
        if (introEssential && countedRC >= ShellsToAdd() && outroStartTime > 0.0f)
        {
            return outroStartTime;
        }
        return loopEndTime;
    }

    [[nodiscard]] float StopJumpTarget() const
    {
        return outroStartTime > 0.0f ? outroStartTime : loopEndTime;
    }
};

inline ReloadState g_reloadState;
}
