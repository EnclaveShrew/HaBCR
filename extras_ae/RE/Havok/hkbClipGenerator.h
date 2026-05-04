#pragma once

#include "RE/H/hkArray.h"
#include "RE/H/hkRefPtr.h"
#include "RE/Havok/hkQsTransform.h"
#include "RE/Havok/hkStringPtr.h"
#include "RE/Havok/hkaAnimation.h"
#include "RE/Havok/hkbGenerator.h"

namespace RE
{
class hkaAnimationBinding;
class hkaDefaultAnimationControl;
class hkaDefaultAnimationControlMapperData;
class hkbClipTriggerArray;
struct hkbContext;

class __declspec(novtable) hkbClipGenerator : public hkbGenerator
{
  public:
    enum class PlaybackMode : std::uint8_t
    {
        kSinglePlay = 0,
        kLooping = 1,
        kUserControlled = 2,
        kPingPong = 3,
        kCount = 4,
    };

    std::uint8_t pad10[0x38 - 0x10];
    hkStringPtr name;
    std::uint8_t pad40[0x50 - 0x40];
    std::uint8_t generatorPartitionInfo[0x28];
    std::uint8_t pad78[0x88 - 0x78];
    hkStringPtr unk88;
    hkStringPtr animationName;
    hkRefPtr<hkbClipTriggerArray> triggers;
    std::uint8_t padA0[0x04];
    float cropStartAmountLocalTime;
    float cropEndAmountLocalTime;
    float startTime;
    float playbackSpeed;
    float enforcedDuration;
    float userControlledTimeFraction;
    std::uint16_t animationBindingIndex;
    PlaybackMode mode;
    std::uint8_t flags;
    std::uint8_t padC0[0x10];
    hkRefPtr<hkaDefaultAnimationControl> animationControl;
    hkRefPtr<hkbClipTriggerArray> originalTriggers;
    hkaDefaultAnimationControlMapperData *mapperData;
    hkaAnimationBinding *binding;
    std::uint8_t padF0[0x10];
    hkQsTransform extractedMotion;
    std::uint8_t pad130[0x08];
    std::uint8_t pad138[0x08];
    float localTime;
    float time;
    float previousUserControlledTimeFraction;
    std::uint8_t pad14C[0x04];
    std::uint16_t unk150;
    bool atEnd;
    bool ignoreStartTime;

    [[nodiscard]] const char *GetClipName() const
    {
        return animationName.get();
    }

    [[nodiscard]] hkaAnimation *GetAnimation() const
    {
        auto ctrl = reinterpret_cast<std::uintptr_t>(animationControl.ptr);
        if (!ctrl)
        {
            return nullptr;
        }
        auto bind = *reinterpret_cast<std::uintptr_t *>(ctrl + 0x38);
        if (!bind)
        {
            return nullptr;
        }
        return *reinterpret_cast<hkaAnimation **>(bind + 0x18);
    }
};
static_assert(offsetof(hkbClipGenerator, name) == 0x38);
static_assert(offsetof(hkbClipGenerator, animationName) == 0x90);
static_assert(offsetof(hkbClipGenerator, triggers) == 0x98);
static_assert(offsetof(hkbClipGenerator, cropStartAmountLocalTime) == 0xA4);
static_assert(offsetof(hkbClipGenerator, playbackSpeed) == 0xB0);
static_assert(offsetof(hkbClipGenerator, animationBindingIndex) == 0xBC);
static_assert(offsetof(hkbClipGenerator, mode) == 0xBE);
static_assert(offsetof(hkbClipGenerator, flags) == 0xBF);
static_assert(offsetof(hkbClipGenerator, animationControl) == 0xD0);
static_assert(offsetof(hkbClipGenerator, binding) == 0xE8);
static_assert(offsetof(hkbClipGenerator, extractedMotion) == 0x100);
static_assert(offsetof(hkbClipGenerator, localTime) == 0x140);
}
