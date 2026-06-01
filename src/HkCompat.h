#pragma once

namespace HaBCR
{
template <class T> inline auto HkData(RE::hkArrayBase<T> &a) noexcept
{
    return a.data;
}

template <class T> inline auto HkData(const RE::hkArrayBase<T> &a) noexcept
{
    return a.data;
}

template <class T> inline std::int32_t HkSize(const RE::hkArrayBase<T> &a) noexcept
{
    return a.size;
}

template <class T> inline T *HkRefGet(const RE::hkRefPtr<T> &r) noexcept
{
    return r.ptr;
}
} // namespace HaBCR
