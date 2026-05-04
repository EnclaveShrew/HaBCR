#pragma once

#include "RE/H/hkArray.h"
#include "RE/Havok/hkStringPtr.h"

namespace RE
{
class hkaAnnotationTrack
{
  public:
    struct Annotation
    {
        float time;
        std::uint32_t pad04;
        hkStringPtr text;
    };
    static_assert(sizeof(Annotation) == 0x10);

    hkStringPtr trackName;
    hkArrayBase<Annotation> annotations;
};
static_assert(sizeof(hkaAnnotationTrack) == 0x18);
}
