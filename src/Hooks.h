#pragma once
#include "ReloadState.h"
#include <Windows.h>

namespace HaBCR
{
template <class Ty> Ty SafeWrite64Function(uintptr_t addr, Ty data)
{
    DWORD oldProtect;
    void *_d[2];
    memcpy(_d, &data, sizeof(data));
    size_t len = sizeof(_d[0]);

    VirtualProtect((void *)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
    Ty olddata;
    memset(&olddata, 0, sizeof(Ty));
    memcpy(&olddata, (void *)addr, len);
    memcpy((void *)addr, &_d[0], len);
    VirtualProtect((void *)addr, len, oldProtect, &oldProtect);
    return olddata;
}

void InstallProcessReloadEventHook(RE::PlayerCharacter *a_player);
void InstallUpdateAnimationHook();
void RegisterEquipEventSink();

void SetWeapAmmoCapacity(std::uint32_t amount);
void ReloadEndHandle();
bool SendStopSignal();
void BCRGotoReloadEnd();

void FillEquipDataFromEquippedItem();
std::uint32_t GetCurrentClipAmmo();

inline RE::ActorValueInfo *g_habcrMarkerAVIF = nullptr;
inline constexpr std::uint32_t kHaBCRMarkerFormID = 0x00FFFFF0;
inline constexpr const char *kHaBCRMarkerEditorID = "HaBCR_Enabled";

void CreateHaBCRMarkerAVIF();
}
