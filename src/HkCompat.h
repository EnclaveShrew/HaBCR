#pragma once

namespace HaBCR
{
template <class T> inline auto HkData(RE::hkArrayBase<T> &a) noexcept
{
#if defined(HABCR_VARIANT_AE)
    return a.data;
#else
    return a._data;
#endif
}

template <class T> inline auto HkData(const RE::hkArrayBase<T> &a) noexcept
{
#if defined(HABCR_VARIANT_AE)
    return a.data;
#else
    return a._data;
#endif
}

template <class T> inline std::int32_t HkSize(const RE::hkArrayBase<T> &a) noexcept
{
#if defined(HABCR_VARIANT_AE)
    return a.size;
#else
    return a._size;
#endif
}

template <class T> inline T *HkRefGet(const RE::hkRefPtr<T> &r) noexcept
{
#if defined(HABCR_VARIANT_AE)
    return r.ptr;
#else
    return r._ptr;
#endif
}
}
