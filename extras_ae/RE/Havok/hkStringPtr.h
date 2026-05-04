#pragma once

namespace RE
{
class hkStringPtr
{
  public:
    [[nodiscard]] const char *c_str() const noexcept
    {
        return reinterpret_cast<const char *>(
            reinterpret_cast<std::uintptr_t>(_data) & ~static_cast<std::uintptr_t>(1));
    }
    [[nodiscard]] const char *get() const noexcept
    {
        return c_str();
    }

    const char *_data;
};
static_assert(sizeof(hkStringPtr) == 0x08);
}
