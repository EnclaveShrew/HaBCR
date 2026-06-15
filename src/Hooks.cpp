#include "Hooks.h"
#include "Settings.h"

namespace HaBCR
{
static void OverwriteLocalTime(RE::hkbClipGenerator *clipGen, float time);
void BCRGotoReloadEnd();

static RE::BSTEventSource<RE::TESEquipEvent> *GetTESEquipEventSource()
{
    if (REX::FModule::IsRuntimeOG())
    {
        static REL::Relocation<RE::BSTEventSource<RE::TESEquipEvent> *> singleton{REL::ID(485633)};
        return singleton.get();
    }

    using func_t = RE::BSTEventSource<RE::TESEquipEvent> &(*)();
    static REL::Relocation<func_t> func{REL::ID(2201838)};
    return std::addressof(func());
}

void ReloadState::Reset()
{
    activeClipGenerator = nullptr;
    startingAmmoCount = 0;
    currentAmmoCount = 0;
    countedRC = 0;
    loopCount = 0;
    inventoryAmmoCount = 0;
    RCPerLoop = 1;
    currentLoopRCCount = 0;
    nextCancelPointIndex = 0;
    beforeLoopRCCount = 0;
    afterLoopRCCount = 0;
    cancelPointTimes.clear();
    loopStartTime = 0.0f;
    loopEndTime = 0.0f;
    outroStartTime = 0.0f;
    phase = ReloadPhase::kIdle;
    mode = ReloadMode::kNone;
    introEssential = false;
    stopPressed = false;
    waitingReloadEnd = false;
    deferredCapacityRestore = 0;
    ammoSwitchInProgress = false;

    if (g_habcrMarkerAVIF)
    {
        auto *player = RE::PlayerCharacter::GetSingleton();
        if (player)
        {
            player->SetBaseActorValue(*g_habcrMarkerAVIF, 0.0f);
        }
    }
}

static void CheckReloadLoopUpdate()
{
    auto &state = g_reloadState;

    if (!state.IsReloading())
        return;
    if (state.mode != ReloadMode::kHaBCR)
        return;
    if (!state.activeClipGenerator)
        return;

    float localTime = state.activeClipGenerator->localTime;

    if (state.stopPressed && state.countedRC > 0 && localTime >= state.loopStartTime && state.currentLoopRCCount == 0)
    {
        LOG_VERBOSE("Stopped at loopStart (shells: {}/{}), jumping to {:.3f}s", state.countedRC, state.ShellsToAdd(),
                    state.StopJumpTarget());
        OverwriteLocalTime(state.activeClipGenerator, state.StopJumpTarget());
        state.phase = ReloadPhase::kIdle;
        ReloadEndHandle();
        return;
    }

    if (state.IsLoopFinished() && localTime >= state.loopStartTime && state.currentLoopRCCount == 0)
    {
        LOG_VERBOSE("Reload complete from intro, skipping to {:.3f}s (shells: {}/{})", state.CompletionJumpTarget(),
                    state.countedRC, state.ShellsToAdd());
        OverwriteLocalTime(state.activeClipGenerator, state.CompletionJumpTarget());
        state.phase = ReloadPhase::kIdle;
        ReloadEndHandle();
        return;
    }

    while (state.nextCancelPointIndex < state.cancelPointTimes.size() &&
           localTime >= state.cancelPointTimes[state.nextCancelPointIndex])
    {
        state.nextCancelPointIndex++;

        if (state.IsLoopFinished())
        {
            LOG_VERBOSE("Complete at CancelPoint (shells: {}/{}), jumping to {:.3f}s", state.countedRC,
                        state.ShellsToAdd(), state.CompletionJumpTarget());
            OverwriteLocalTime(state.activeClipGenerator, state.CompletionJumpTarget());
            state.phase = ReloadPhase::kIdle;
            ReloadEndHandle();
            return;
        }
        if (state.stopPressed)
        {
            LOG_VERBOSE("Stopped at CancelPoint (shells: {}/{}), jumping to {:.3f}s", state.countedRC,
                        state.ShellsToAdd(), state.StopJumpTarget());
            OverwriteLocalTime(state.activeClipGenerator, state.StopJumpTarget());
            state.phase = ReloadPhase::kIdle;
            ReloadEndHandle();
            return;
        }
    }

    if (localTime >= state.loopEndTime)
    {
        if (state.IsLoopFinished())
        {
            LOG_VERBOSE("Reload complete (shells: {}/{}), letting animation continue to outro", state.countedRC,
                        state.ShellsToAdd());
            state.phase = ReloadPhase::kIdle;
            ReloadEndHandle();
            return;
        }
        if (state.stopPressed)
        {
            if (state.outroStartTime > 0.0f)
            {
                LOG_VERBOSE("Reload stopped (shells: {}/{}), jumping to outroStart {:.3f}s", state.countedRC,
                            state.ShellsToAdd(), state.outroStartTime);
                OverwriteLocalTime(state.activeClipGenerator, state.outroStartTime);
            }
            else
            {
                LOG_VERBOSE("Reload stopped (shells: {}/{}), letting animation continue", state.countedRC,
                            state.ShellsToAdd());
            }
            state.phase = ReloadPhase::kIdle;
            ReloadEndHandle();
            return;
        }

        state.loopCount++;
        float beforeTime = state.activeClipGenerator->localTime;
        state.currentLoopRCCount = 0;
        state.nextCancelPointIndex = 0;
        OverwriteLocalTime(state.activeClipGenerator, state.loopStartTime);
        float afterTime = state.activeClipGenerator->localTime;
        LOG_VERBOSE("Rewind #{}: target={:.4f}, before={:.4f}, after={:.4f} (shells: {}/{}, per loop: {})",
                    state.loopCount, state.loopStartTime, beforeTime, afterTime, state.countedRC, state.ShellsToAdd(),
                    state.RCPerLoop);
    }
}

struct UpdateAnimationHook
{
    static void thunk(RE::PlayerCharacter *a_this, float a_delta)
    {
        func(a_this, a_delta);
        CheckReloadLoopUpdate();
        if (g_reloadState.deferredCapacityRestore > 0)
        {
            SetWeapAmmoCapacity(g_reloadState.deferredCapacityRestore);
            g_reloadState.deferredCapacityRestore = 0;
        }
    }

    inline static REL::Relocation<decltype(thunk)> func;

    static void Install()
    {
        REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE::PlayerCharacter[0]};
        func = vtbl.write_vfunc(0x9F, thunk);
        LOG_INFO("UpdateAnimation hook installed");
    }
};

void InstallUpdateAnimationHook()
{
    UpdateAnimationHook::Install();
}

static constexpr std::string_view kLoopStartAnnotation = "HaBCR_LoopStart";
static constexpr std::string_view kLoopEndAnnotation = "HaBCR_LoopEnd";
static constexpr std::string_view kCancelPointAnnotation = "HaBCR_CancelPoint";
static constexpr std::string_view kOutroStartAnnotation = "HaBCR_OutroStart";
static constexpr std::string_view kIntroEssentialAnnotation = "HaBCR_IntroEssential";
static constexpr std::string_view kEvent00Annotation = "event00";
static constexpr std::string_view kReloadCompleteAnnotation = "reloadComplete";

void FillEquipDataFromEquippedItem()
{
    g_reloadState.weapInstance = nullptr;
    g_reloadState.ammoCapacity = 0;
    g_reloadState.equippedWeaponFormID = 0;

    auto *player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->currentProcess || !player->currentProcess->middleHigh)
    {
        return;
    }

    auto &equipped = player->currentProcess->middleHigh->equippedItems;
    for (auto &item : equipped)
    {
        if (!item.data)
        {
            continue;
        }

        auto vtable = *reinterpret_cast<std::uintptr_t *>(item.data.get());
        if (vtable != RE::VTABLE::EquippedWeaponData[0].address())
        {
            continue;
        }

        RE::TESObjectWEAP::InstanceData *inst = nullptr;
        if (item.item.instanceData)
        {
            inst = static_cast<RE::TESObjectWEAP::InstanceData *>(item.item.instanceData.get());
        }
        else if (item.item.object && item.item.object->GetFormType() == RE::ENUM_FORM_ID::kWEAP)
        {
            inst = &static_cast<RE::TESObjectWEAP *>(item.item.object)->weaponData;
        }

        if (!inst)
        {
            continue;
        }

        g_reloadState.weapInstance = inst;
        g_reloadState.ammoCapacity = inst->ammoCapacity;
        if (item.item.object && item.item.object->GetFormType() == RE::ENUM_FORM_ID::kWEAP)
        {
            g_reloadState.equippedWeaponFormID = item.item.object->GetFormID();
        }
        return;
    }
}

void CreateHaBCRMarkerAVIF()
{
    if (g_habcrMarkerAVIF)
    {
        return;
    }

    auto *factory = RE::ConcreteFormFactory<RE::ActorValueInfo, RE::ENUM_FORM_ID::kAVIF>::GetFormFactory();
    if (!factory)
    {
        logger::error("Failed to get AVIF form factory");
        return;
    }

    auto *avif = factory->Create();
    if (!avif)
    {
        logger::error("Failed to create AVIF instance");
        return;
    }

    avif->SetFormID(kHaBCRMarkerFormID, false);
    avif->SetFormEditorID(kHaBCRMarkerEditorID);

    auto *dh = RE::TESDataHandler::GetSingleton();
    if (!dh || !dh->AddFormToDataHandler(avif))
    {
        logger::error("Failed to register HaBCR marker AVIF in data handler");
        return;
    }

    g_habcrMarkerAVIF = avif;
    LOG_INFO("HaBCR marker AVIF created, formID=0x{:08X}, editorID=\"{}\"", kHaBCRMarkerFormID, kHaBCRMarkerEditorID);
}

std::uint32_t GetCurrentClipAmmo()
{
    auto *player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->currentProcess || !player->currentProcess->middleHigh)
    {
        logger::warn("Player/process not available");
        return 0;
    }

    auto &equippedItems = player->currentProcess->middleHigh->equippedItems;

    for (auto &equipped : equippedItems)
    {
        if (!equipped.data)
        {
            continue;
        }

        auto *itemData = equipped.data.get();
        auto vtable = *reinterpret_cast<std::uintptr_t *>(itemData);
        if (vtable != RE::VTABLE::EquippedWeaponData[0].address())
        {
            continue;
        }

        auto *weaponData = static_cast<RE::EquippedWeaponData *>(itemData);
        return weaponData->ammoCount;
    }

    logger::warn("No equipped weapon");
    return 0;
}

struct AnnotationScanResult
{
    bool foundLoopStart{false};
    bool foundLoopEnd{false};
    bool foundEvent00{false};
    float loopStartTime{0.0f};
    float loopEndTime{0.0f};
    std::uint32_t RCPerLoop{1};
    std::uint32_t beforeLoopRCCount{0};
    std::uint32_t afterLoopRCCount{0};
    float outroStartTime{0.0f};
    bool foundOutroStart{false};
    bool introEssential{false};
    std::vector<float> cancelPointTimes;
    RE::hkbClipGenerator *clipGenerator{nullptr};

    bool IsValid() const
    {
        return foundLoopStart && foundLoopEnd;
    }

    bool IsBCR() const
    {
        return foundEvent00 && !IsValid();
    }
};

struct ActiveGraphInfo
{
    RE::hkbCharacter *character{nullptr};
    RE::hkArray<RE::hkbNodeInfo *> *activeNodes{nullptr};
};

static ActiveGraphInfo GetActiveGraphInfo(RE::PlayerCharacter *a_player)
{
    RE::BSTSmartPointer<RE::BSAnimationGraphManager> graphMgr;
    if (!a_player->GetAnimationGraphManagerImpl(graphMgr) || !graphMgr)
    {
        return {};
    }

    std::uint32_t activeIdx = graphMgr->activeGraph;
    if (activeIdx >= graphMgr->graph.size())
    {
        return {};
    }

    auto &bsGraph = graphMgr->graph[activeIdx];
    if (!bsGraph)
    {
        return {};
    }

    auto &character = bsGraph->QCharacter();
    auto *behaviorGraph = character.behaviorGraph;
    if (!behaviorGraph)
    {
        return {};
    }

    auto *nodes = behaviorGraph->GetActiveNodes();
    if (!nodes || !HkData(*nodes) || HkSize(*nodes) <= 0)
    {
        return {};
    }

    return {&character, nodes};
}

static bool IsInReloadState(RE::PlayerCharacter *a_player)
{
    static REL::Relocation<std::uintptr_t> kSMVtable{RE::hkbStateMachine::VTABLE[0]};

    auto info = GetActiveGraphInfo(a_player);
    if (!info.activeNodes)
        return false;

    for (int i = 0; i < HkSize(*info.activeNodes); ++i)
    {
        auto *nodeInfo = HkData(*info.activeNodes)[i];
        if (!nodeInfo || !nodeInfo->nodeClone)
            continue;

        if (*reinterpret_cast<std::uintptr_t *>(nodeInfo->nodeClone) != kSMVtable.address())
            continue;

        auto *sm = static_cast<RE::hkbStateMachine *>(nodeInfo->nodeClone);
        auto stateId = sm->GetCurrentStateId();
        if (stateId == 4 || stateId == 44)
            return true;
    }
    return false;
}

static AnnotationScanResult ScanClipAnnotations(RE::hkbClipGenerator *clipGen)
{
    AnnotationScanResult result;

    auto *anim = clipGen->GetAnimation();
    if (!anim)
    {
        return result;
    }

    auto &tracks = anim->annotationTracks;
    if (!HkData(tracks) || HkSize(tracks) <= 0)
    {
        return result;
    }

    std::vector<float> reloadCompleteTimes;
    std::vector<float> cancelPointTimes;

    for (std::int32_t t = 0; t < HkSize(tracks); ++t)
    {
        auto &track = HkData(tracks)[t];
        if (!HkData(track.annotations) || HkSize(track.annotations) <= 0)
        {
            continue;
        }

        for (std::int32_t a = 0; a < HkSize(track.annotations); ++a)
        {
            auto &annot = HkData(track.annotations)[a];
            const char *text = annot.text.get();
            if (!text || !*text)
            {
                continue;
            }

            if (_stricmp(text, kLoopStartAnnotation.data()) == 0)
            {
                if (result.foundLoopStart)
                {
                    LOG_INFO("Duplicate HaBCR_LoopStart annotation at {:.3f}s and {:.3f}s", result.loopStartTime,
                             annot.time);
                }
                result.foundLoopStart = true;
                result.loopStartTime = annot.time;
            }
            else if (_stricmp(text, kLoopEndAnnotation.data()) == 0)
            {
                if (result.foundLoopEnd)
                {
                    LOG_INFO("Duplicate HaBCR_LoopEnd annotation at {:.3f}s and {:.3f}s", result.loopEndTime,
                             annot.time);
                }
                result.foundLoopEnd = true;
                result.loopEndTime = annot.time;
            }
            else if (_stricmp(text, kCancelPointAnnotation.data()) == 0)
            {
                cancelPointTimes.push_back(annot.time);
            }
            else if (_stricmp(text, kOutroStartAnnotation.data()) == 0)
            {
                result.foundOutroStart = true;
                result.outroStartTime = annot.time;
            }
            else if (_stricmp(text, kEvent00Annotation.data()) == 0)
            {
                result.foundEvent00 = true;
            }
            else if (_stricmp(text, kReloadCompleteAnnotation.data()) == 0)
            {
                reloadCompleteTimes.push_back(annot.time);
            }
            else if (_stricmp(text, kIntroEssentialAnnotation.data()) == 0)
            {
                result.introEssential = true;
            }
        }
    }

    if (result.IsValid())
    {
        std::uint32_t beforeLoop = 0;
        std::uint32_t inLoop = 0;
        std::uint32_t afterLoop = 0;
        for (float t : reloadCompleteTimes)
        {
            if (t < result.loopStartTime)
                ++beforeLoop;
            else if (t <= result.loopEndTime)
                ++inLoop;
            else
                ++afterLoop;
        }
        result.RCPerLoop = inLoop > 0 ? inLoop : 1;
        result.beforeLoopRCCount = beforeLoop;
        result.afterLoopRCCount = afterLoop;

        result.cancelPointTimes = std::move(cancelPointTimes);
        std::sort(result.cancelPointTimes.begin(), result.cancelPointTimes.end());
    }

    return result;
}

static AnnotationScanResult ScanActiveClipAnnotations(RE::PlayerCharacter *a_player)
{
    auto info = GetActiveGraphInfo(a_player);
    if (!info.activeNodes)
    {
        return {};
    }

    AnnotationScanResult best;

    for (std::int32_t i = 0; i < HkSize(*info.activeNodes); ++i)
    {
        auto *nodeInfo = HkData(*info.activeNodes)[i];
        if (!nodeInfo || !nodeInfo->nodeClone)
        {
            continue;
        }

        auto vtable = *reinterpret_cast<std::uintptr_t *>(nodeInfo->nodeClone);
        if (vtable != RE::VTABLE::hkbClipGenerator[0].address())
        {
            continue;
        }

        auto *clip = static_cast<RE::hkbClipGenerator *>(nodeInfo->nodeClone);
        auto result = ScanClipAnnotations(clip);

        if (result.IsValid())
        {
            result.clipGenerator = clip;
            return result;
        }

        if (result.foundEvent00)
        {
            best.foundEvent00 = true;
        }
    }

    return best;
}

static void OverwriteLocalTime(RE::hkbClipGenerator *clipGen, float time)
{
    if (!clipGen)
    {
        logger::error("OverwriteLocalTime - clipGen is null");
        return;
    }

    clipGen->localTime = time;

    auto *animCtrl = HkRefGet(clipGen->animationControl);
    if (animCtrl)
    {
        *reinterpret_cast<float *>(reinterpret_cast<std::uintptr_t>(animCtrl) + 0x10) = time;
    }

    *reinterpret_cast<std::uint8_t *>(reinterpret_cast<std::uintptr_t>(clipGen) + 0x151) = 1;
}

void SetWeapAmmoCapacity(std::uint32_t amount)
{
    auto &state = g_reloadState;
    if (state.weapInstance)
    {
        if (amount > state.ammoCapacity)
        {
            state.weapInstance->ammoCapacity = state.ammoCapacity;
        }
        else
        {
            state.weapInstance->ammoCapacity = static_cast<std::uint16_t>(amount);
        }
    }
}

void ReloadEndHandle()
{
    auto &state = g_reloadState;

    SetWeapAmmoCapacity(state.ammoCapacity);

    LOG_INFO("Reload ended (shells inserted: {}/{})", state.countedRC, state.ShellsToAdd());
    state.Reset();
}

bool SendStopSignal()
{
    auto *player = RE::PlayerCharacter::GetSingleton();
    if (!player)
    {
        return false;
    }

    static RE::BSFixedString reloadEnd{"reloadEnd"};
    static RE::BSFixedString reloadEndSlave{"reloadEndSlave"};

    bool result = player->NotifyAnimationGraphImpl(reloadEnd);
    player->NotifyAnimationGraphImpl(reloadEndSlave);

    return result;
}

void BCRGotoReloadEnd()
{
    bool result = SendStopSignal();

    if (result)
    {
        ReloadEndHandle();
    }
    else
    {
        logger::warn("SendStopSignal failed");
    }
}

class AnimationGraphEventWatcher : public RE::BSTEventSink<RE::BSAnimationGraphEvent>
{
  public:
    using FnProcessEvent = RE::BSEventNotifyControl (AnimationGraphEventWatcher::*)(
        RE::BSAnimationGraphEvent &a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent> *a_source);

    RE::BSEventNotifyControl HookedProcessEvent(RE::BSAnimationGraphEvent &a_event,
                                                RE::BSTEventSource<RE::BSAnimationGraphEvent> *a_source)
    {
        ProcessReloadEvent(a_event);

        return originalFn ? (this->*originalFn)(a_event, a_source) : RE::BSEventNotifyControl::kContinue;
    }

    void HookSink()
    {
        if (!originalFn)
        {
            std::uint64_t vtable = *(std::uint64_t *)this;
            originalFn = SafeWrite64Function(vtable + 0x8, &AnimationGraphEventWatcher::HookedProcessEvent);
        }
    }

  private:
    void ProcessReloadEvent(const RE::BSAnimationGraphEvent &a_event)
    {
        auto &state = g_reloadState;
        auto *player = RE::PlayerCharacter::GetSingleton();

        if (!player || !state.weapInstance)
        {
            return;
        }

        const char *tag = a_event.tag.c_str();
        if (!tag || !*tag)
        {
            return;
        }

        if (state.phase == ReloadPhase::kIdle)
        {
            if (_stricmp("reloadStateEnter", tag) == 0)
            {
                if (!IsInReloadState(player))
                {
                    return;
                }

                state.phase = ReloadPhase::kStarted;

                FillEquipDataFromEquippedItem();

                std::uint32_t currentAmmo = GetCurrentClipAmmo();
                state.startingAmmoCount = currentAmmo;
                state.currentAmmoCount = currentAmmo;

                state.inventoryAmmoCount = 0;
                if (state.weapInstance && state.weapInstance->ammo)
                {
                    std::uint32_t count = 0;
                    if (player->GetItemCount(count, state.weapInstance->ammo, false))
                    {
                        state.inventoryAmmoCount = count > currentAmmo ? count - currentAmmo : 0;
                    }
                }

                state.countedRC = 0;
                state.stopPressed = false;
                state.currentLoopRCCount = 0;
                state.RCPerLoop = 1;
                state.mode = ReloadMode::kNone;

                LOG_VERBOSE("Reload started (ammo: {}/{}, tactical: {})", currentAmmo, state.ammoCapacity,
                            (currentAmmo > 0));
                return;
            }
        }

        if (state.phase == ReloadPhase::kStarted && state.ShellsToAdd() > 0)
        {
            auto scan = ScanActiveClipAnnotations(player);

            if (scan.IsValid())
            {
                state.mode = ReloadMode::kHaBCR;
                state.phase = ReloadPhase::kLooping;
                state.loopStartTime = scan.loopStartTime;
                state.loopEndTime = scan.loopEndTime;
                state.RCPerLoop = scan.RCPerLoop;
                state.cancelPointTimes = scan.cancelPointTimes;
                state.activeClipGenerator = scan.clipGenerator;
                state.beforeLoopRCCount = scan.beforeLoopRCCount;
                state.afterLoopRCCount = scan.afterLoopRCCount;
                state.outroStartTime = scan.outroStartTime;
                state.introEssential = scan.introEssential;
                state.currentLoopRCCount = 0;
                state.nextCancelPointIndex = 0;

                SetWeapAmmoCapacity(state.currentAmmoCount);

                if (g_habcrMarkerAVIF)
                {
                    player->SetBaseActorValue(*g_habcrMarkerAVIF, 1.0f);
                    LOG_INFO("MarkerAV -> 1.0");
                }

                LOG_INFO("HaBCR mode");
                LOG_VERBOSE("  shells to add: {}, loop: {:.3f}s->{:.3f}s, RC: before={} loop={} after={}, "
                            "outroStart: {:.3f}s, cancel points: {}, introEssential: {}",
                            state.ShellsToAdd(), state.loopStartTime, state.loopEndTime, state.beforeLoopRCCount,
                            state.RCPerLoop, state.afterLoopRCCount, state.outroStartTime,
                            state.cancelPointTimes.size(), state.introEssential);
            }
            else if (Settings::GetSingleton()->bcrCompatible && scan.foundEvent00)
            {
                state.mode = ReloadMode::kBCR;
                state.phase = ReloadPhase::kLooping;
                SetWeapAmmoCapacity(state.currentAmmoCount);

                if (g_habcrMarkerAVIF)
                {
                    player->SetBaseActorValue(*g_habcrMarkerAVIF, 1.0f);
                    LOG_INFO("MarkerAV -> 1.0");
                }

                LOG_INFO("BCR mode");
                LOG_VERBOSE("  shells to add: {}", state.ShellsToAdd());
            }
        }

        if (_stricmp("reloadEnd", tag) == 0)
        {
            if (IsInReloadState(player) && state.IsReloading())
            {
                ReloadEndHandle();
            }
            return;
        }

        if (_stricmp("CullBone", tag) == 0)
        {
            if (state.mode == ReloadMode::kBCR && state.waitingReloadEnd &&
                (state.IsTacticalReloading() || state.currentAmmoCount < state.ammoCapacity))
            {
                BCRGotoReloadEnd();
            }
            return;
        }

        if (!state.IsReloading())
        {
            return;
        }

        if (_stricmp("reloadComplete", tag) == 0)
        {
            switch (state.mode)
            {
            case ReloadMode::kHaBCR: {
                bool essentialIntroOverride = state.introEssential && state.countedRC < state.beforeLoopRCCount &&
                                              state.countedRC < state.ShellsToAdd();

                if ((!state.IsLoopFinished() || essentialIntroOverride) && !state.stopPressed)
                {
                    state.countedRC++;
                    state.currentAmmoCount = state.startingAmmoCount + state.countedRC;
                    SetWeapAmmoCapacity(state.currentAmmoCount);

                    float clipTime = state.activeClipGenerator ? state.activeClipGenerator->localTime : 0.0f;
                    bool inLoopRange = clipTime >= state.loopStartTime;

                    if (inLoopRange && state.currentLoopRCCount < state.RCPerLoop)
                    {
                        state.currentLoopRCCount++;
                    }
                }
                break;
            }

            case ReloadMode::kBCR:
                if (state.waitingReloadEnd)
                {
                    return;
                }
                if (state.stopPressed)
                {
                    const auto restoreCapacity = state.ammoCapacity;
                    if (!SendStopSignal())
                    {
                        logger::warn("SendStopSignal failed");
                    }
                    LOG_INFO("BCR reload stopped (shells inserted: {}/{})", state.countedRC, state.ShellsToAdd());
                    state.Reset();
                    state.deferredCapacityRestore = restoreCapacity;
                    return;
                }

                state.countedRC++;
                state.currentAmmoCount = state.startingAmmoCount + state.countedRC;
                SetWeapAmmoCapacity(state.currentAmmoCount);

                if (state.IsLoopFinished())
                {
                    state.waitingReloadEnd = true;
                    SetWeapAmmoCapacity(state.currentAmmoCount);
                }
                break;

            default:
                break;
            }

            return;
        }

        if (_stricmp("pipboyOpened", tag) == 0 || _stricmp("weaponSwing", tag) == 0 || _stricmp("throwEnd", tag) == 0 ||
            _stricmp("weaponDraw", tag) == 0 || _stricmp("impactLandEnd", tag) == 0)
        {
            LOG_INFO("Reload interrupted by {}", tag);
            ReloadEndHandle();
            return;
        }
    }

    static inline FnProcessEvent originalFn = nullptr;
};

void InstallProcessReloadEventHook(RE::PlayerCharacter *a_player)
{
    auto *watcher = reinterpret_cast<AnimationGraphEventWatcher *>(reinterpret_cast<std::uintptr_t>(a_player) + 0x38);
    watcher->HookSink();
    LOG_INFO("Animation event hook installed");
}

class EquipEventSink : public RE::BSTEventSink<RE::TESEquipEvent>
{
  public:
    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent &a_event,
                                          RE::BSTEventSource<RE::TESEquipEvent> *) override
    {
        auto *player = RE::PlayerCharacter::GetSingleton();
        auto *eventActor = a_event.actor.get();
        const bool eventEquipped = a_event.equipped;
        const std::uint32_t eventFormId = a_event.baseObject;

        if (!player || eventActor != player)
        {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (!eventEquipped)
        {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto *form = RE::TESForm::GetFormByID(eventFormId);
        if (!form || form->GetFormType() != RE::ENUM_FORM_ID::kWEAP)
        {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto *weap = static_cast<RE::TESObjectWEAP *>(form);

        LOG_INFO("Equipped weapon: {} (FormID: {:08X})", weap->GetFullName(), weap->GetFormID());

        if (g_reloadState.ammoSwitchInProgress && g_reloadState.IsReloading() &&
            g_reloadState.equippedWeaponFormID != 0 && eventFormId == g_reloadState.equippedWeaponFormID)
        {
            FillEquipDataFromEquippedItem();
            LOG_INFO("Ignored same-weapon equip event during ammo switch");
            return RE::BSEventNotifyControl::kContinue;
        }

        if (g_reloadState.IsReloading())
        {
            ReloadEndHandle();
        }

        FillEquipDataFromEquippedItem();

        return RE::BSEventNotifyControl::kContinue;
    }

    static EquipEventSink *GetSingleton()
    {
        static EquipEventSink singleton;
        return &singleton;
    }
};

void RegisterEquipEventSink()
{
    auto *source = GetTESEquipEventSource();
    if (source)
    {
        source->RegisterSink(EquipEventSink::GetSingleton());
        LOG_INFO("EquipEvent sink registered");
    }
    else
    {
        logger::error("Failed to get EquipEventSource");
    }
}

} // namespace HaBCR
