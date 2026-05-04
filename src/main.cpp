#include "Hooks.h"
#include "InputHandler.h"
#include "Settings.h"

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

#if defined(HABCR_VARIANT_OG)
static bool SetupLogger()
{
#ifndef NDEBUG
    auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
    auto path = logger::log_directory();
    if (!path)
    {
        return false;
    }

    *path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
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
#endif

#if defined(HABCR_VARIANT_AE)

#elif defined(HABCR_VARIANT_OG)

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
    if (ver != F4SE::RUNTIME_1_10_163)
    {
        logger::critical("unsupported runtime v{}", ver.string());
        return false;
    }

    return true;
}

#else
#error "HABCR_VARIANT_OG or HABCR_VARIANT_AE must be defined via CMake"
#endif

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface *a_f4se)
{
#if defined(HABCR_VARIANT_AE)
    F4SE::Init(a_f4se, F4SE::InitInfo{
                           .log = true,
                           .logName = "HaBCR",
                       });

    const auto ver = a_f4se->RuntimeVersion();
    if (ver != F4SE::RUNTIME_1_11_191)
    {
        logger::critical("unsupported runtime v{}", ver.string());
        return false;
    }
#else
    F4SE::Init(a_f4se);
#endif

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
