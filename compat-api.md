# HaBCR Compatibility API

HaBCR exports a small C ABI for mods that need to coordinate reload state during tactical reload or ammo switching. Consumers should load these functions with `GetProcAddress`.

## Reload Mode

```cpp
uint32_t HaBCR_GetCurrentReloadMode();
```

Return values:

- `0`: HaBCR is not currently handling a reload
- `1`: HaBCR annotation mode
- `2`: HaBCR BCR-compatible mode

HaBCR does not report original BCR state through this API.

This is an active reload query. HaBCR detects the mode by scanning the active reload animation clip after `reloadStateEnter`, so this function does not report whether an equipped weapon will support HaBCR before its reload animation starts.

## BCR-Compatible Setting

```cpp
bool HaBCR_IsBCRCompatibleEnabled();
```

Returns the current `bBCRCompatible` setting loaded from `HaBCR.ini`. Consumers can use this with their own BCR weapon detection or cache. HaBCR does not provide a pre-reload HaBCR weapon capability query.

## Ammo Capacity

```cpp
bool HaBCR_SetAmmoCapacity(uint32_t capacity);
```

Updates HaBCR's logical ammo capacity. The value must fit in `uint16_t`. During an active reload, HaBCR reapplies its temporary weapon capacity from the current loaded ammo count.

## Ammo Switch Flow

```cpp
bool HaBCR_BeginAmmoSwitch();
bool HaBCR_UpdateAmmoStateAfterSwitch(
    uint32_t loadedAmmoCount,
    uint32_t ammoCapacity,
    uint32_t totalAmmoCount);
bool HaBCR_EndAmmoSwitch();
```

Recommended order:

1. Call `HaBCR_BeginAmmoSwitch()` before switching ammo.
2. Perform the ammo switch.
3. Call `HaBCR_UpdateAmmoStateAfterSwitch()` with the new ammo state.
4. Call `HaBCR_EndAmmoSwitch()`.

While ammo switch suppress is active, HaBCR ignores same-weapon equip events so the running reload sequence is not reset. A different weapon equip event still ends the reload normally.

During a suppressed same-weapon equip event, HaBCR refreshes the equipped weapon instance data so ammo switches do not leave the compatibility API using a stale weapon instance pointer.

`totalAmmoCount` means loaded ammo plus reserve ammo. HaBCR converts it to reserve ammo with `max(totalAmmoCount - loadedAmmoCount, 0)`.
