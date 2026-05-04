#pragma once

#include "RE/H/hkArray.h"
#include "RE/H/hkRefPtr.h"
#include "RE/H/hkReferencedObject.h"
#include "RE/Havok/hkaAnnotationTrack.h"

namespace RE
{
class hkaAnimatedReferenceFrame;

class __declspec(novtable) hkaAnimation : public hkReferencedObject
{
  public:
    enum class AnimationType : std::int32_t
    {
        kUnknown = 0,
        kInterleaved = 1,
        kMirrored = 4,
        kSpline = 5,
        kQuantized = 6,
        kPredictive = 7,
    };

    AnimationType type;
    float duration;
    std::int32_t numberOfTransformTracks;
    std::int32_t numberOfFloatTracks;
    hkRefPtr<hkaAnimatedReferenceFrame> extractedMotion;
    hkArrayBase<hkaAnnotationTrack> annotationTracks;
};
static_assert(offsetof(hkaAnimation, duration) == 0x14);
static_assert(offsetof(hkaAnimation, annotationTracks) == 0x28);
}
