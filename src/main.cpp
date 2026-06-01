#include "Hooks.h"
#include "InputHandler.h"
#include "Settings.h"

namespace
{
[[nodiscard]] bool IsSupportedRuntime(const REL::Version &a_version)
{
    return a_version == F4SE::RUNTIME_1_10_163 || a_version == F4SE::RUNTIME_1_11_221;
}

[[nodiscard]] bool SetupLogger()
{
#ifndef NDEBUG
    auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
    auto path = logger::log_directory();
    if (!path)
    {
        return false;
    }

    *path /= std::format("{}.log", Version::PROJECT);
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

    auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
    log->set_level(spdlog::level::trace);
#else
    log->set_level(spdlog::level::info);
    log->flush_on(spdlog::level::info);
#endif

    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

    logger::info("{} v{}", Version::PROJECT, Version::NAME);
    return true;
}
} // namespace

static void MessageCallback(F4SE::MessagingInterface::Message *a_msg)
{
    switch (a_msg->type)
    {
    case F4SE::MessagingInterface::kGameDataReady: {
        auto *player = RE::PlayerCharacter::GetSingleton();
        if (!player)
        {
            logger::error("Failed to get PlayerCharacter singleton");
            return;
        }

        HaBCR::InstallProcessReloadEventHook(player);
        HaBCR::InstallUpdateAnimationHook();
        HaBCR::RegisterEquipEventSink();
        HaBCR::InstallInputHandler();
        HaBCR::CreateHaBCRMarkerAVIF();

        LOG_INFO("Hooks installed");
    }
    break;

    case F4SE::MessagingInterface::kNewGame: {
        HaBCR::g_reloadState.Reset();
        LOG_INFO("New game");
    }
    break;

    case F4SE::MessagingInterface::kPostLoadGame: {
        HaBCR::g_reloadState.Reset();
        LOG_INFO("Game loaded");
        HaBCR::FillEquipDataFromEquippedItem();
    }
    break;
    }
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface *a_f4se, F4SE::PluginInfo *a_info)
{
    if (!SetupLogger())
    {
        return false;
    }

    a_info->infoVersion = F4SE::PluginInfo::kVersion;
    a_info->name = Version::PROJECT.data();
    a_info->version = Version::MAJOR;

    if (a_f4se->IsEditor())
    {
        logger::critical("loaded in editor");
        return false;
    }

    const auto ver = a_f4se->RuntimeVersion();
    if (!IsSupportedRuntime(ver))
    {
        logger::critical("unsupported runtime v{}", ver.string());
        return false;
    }

    return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface *a_f4se)
{
    F4SE::Init(a_f4se);

    HaBCR::Settings::GetSingleton()->Load();

    const auto *messaging = F4SE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(MessageCallback))
    {
        logger::critical("Failed to register messaging listener");
        return false;
    }

    logger::info("Plugin loaded");
    return true;
}
