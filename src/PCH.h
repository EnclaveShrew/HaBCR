#pragma once

#pragma warning(push)
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#if defined(HABCR_VARIANT_AE)
#include "RE/Bethesda/BShkbAnimationGraph.h"
#include "RE/Bethesda/EquipEventSource.h"
#include "RE/Havok/hkaAnimation.h"
#include "RE/Havok/hkaAnnotationTrack.h"
#include "RE/Havok/hkbBehaviorGraph.h"
#include "RE/Havok/hkbCharacter.h"
#include "RE/Havok/hkbClipGenerator.h"
#include "RE/Havok/hkbNodeInfo.h"
#include "RE/Havok/hkbStateMachine.h"
#endif

#include "../../Utilities/SimpleIni.h"

#ifdef NDEBUG
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <spdlog/sinks/msvc_sink.h>
#endif
#pragma warning(pop)

#define DLLEXPORT __declspec(dllexport)

#if defined(HABCR_VARIANT_AE)

namespace logger
{
template <class... T> struct info
{
    info() = delete;
    explicit info(std::format_string<T...> a_fmt, T &&...a_args,
                  std::source_location a_loc = std::source_location::current())
    {
        REX::Impl::Log(a_loc, REX::ELogLevel::Info, a_fmt, std::forward<T>(a_args)...);
    }
};
template <class... T> info(std::format_string<T...>, T &&...) -> info<T...>;

template <class... T> struct error
{
    error() = delete;
    explicit error(std::format_string<T...> a_fmt, T &&...a_args,
                   std::source_location a_loc = std::source_location::current())
    {
        REX::Impl::Log(a_loc, REX::ELogLevel::Error, a_fmt, std::forward<T>(a_args)...);
    }
};
template <class... T> error(std::format_string<T...>, T &&...) -> error<T...>;

template <class... T> struct critical
{
    critical() = delete;
    explicit critical(std::format_string<T...> a_fmt, T &&...a_args,
                      std::source_location a_loc = std::source_location::current())
    {
        REX::Impl::Log(a_loc, REX::ELogLevel::Critical, a_fmt, std::forward<T>(a_args)...);
    }
};
template <class... T> critical(std::format_string<T...>, T &&...) -> critical<T...>;

template <class... T> struct warn
{
    warn() = delete;
    explicit warn(std::format_string<T...> a_fmt, T &&...a_args,
                  std::source_location a_loc = std::source_location::current())
    {
        REX::Impl::Log(a_loc, REX::ELogLevel::Warning, a_fmt, std::forward<T>(a_args)...);
    }
};
template <class... T> warn(std::format_string<T...>, T &&...) -> warn<T...>;

template <class... T> struct debug
{
    debug() = delete;
    explicit debug(std::format_string<T...> a_fmt, T &&...a_args,
                   std::source_location a_loc = std::source_location::current())
    {
        REX::Impl::Log(a_loc, REX::ELogLevel::Debug, a_fmt, std::forward<T>(a_args)...);
    }
};
template <class... T> debug(std::format_string<T...>, T &&...) -> debug<T...>;
}

#else

namespace logger = F4SE::log;

#endif

using namespace std::literals;

#include "Version.h"

#include "HkCompat.h"
