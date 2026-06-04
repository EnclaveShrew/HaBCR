#pragma once
#include <cstdint>

#ifndef HABCR_COMPAT_API
#ifdef HaBCR_EXPORTS
#define HABCR_COMPAT_API __declspec(dllexport)
#else
#define HABCR_COMPAT_API __declspec(dllimport)
#endif
#endif

extern "C" HABCR_COMPAT_API std::uint32_t HaBCR_GetCurrentReloadMode();
extern "C" HABCR_COMPAT_API bool HaBCR_SetAmmoCapacity(std::uint32_t a_capacity);
extern "C" HABCR_COMPAT_API bool HaBCR_BeginAmmoSwitch();
extern "C" HABCR_COMPAT_API bool HaBCR_EndAmmoSwitch();
extern "C" HABCR_COMPAT_API bool HaBCR_UpdateAmmoStateAfterSwitch(std::uint32_t a_loadedAmmoCount,
                                                                  std::uint32_t a_ammoCapacity,
                                                                  std::uint32_t a_totalAmmoCount);
