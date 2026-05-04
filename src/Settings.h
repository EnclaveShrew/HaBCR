#pragma once

namespace HaBCR
{
struct Settings
{
    bool enableLog{true};
    bool verboseLog{false};
    bool bcrCompatible{true};

    static Settings *GetSingleton()
    {
        static Settings singleton;
        return &singleton;
    }

    void Load()
    {
        constexpr auto path = L"Data/F4SE/Plugins/HaBCR.ini";

        CSimpleIniA ini;
        ini.SetUnicode();

        if (ini.LoadFile(path) < 0)
        {
            return;
        }

        enableLog = ini.GetBoolValue("Log", "bEnableLog", true);
        verboseLog = ini.GetBoolValue("Log", "bVerboseLog", false);

        bcrCompatible = ini.GetBoolValue("General", "bBCRCompatible", true);
    }
};

#define LOG_INFO(...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (HaBCR::Settings::GetSingleton()->enableLog)                                                                \
            logger::info(__VA_ARGS__);                                                                                 \
    } while (0)

#define LOG_VERBOSE(...)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (HaBCR::Settings::GetSingleton()->enableLog && HaBCR::Settings::GetSingleton()->verboseLog)                 \
            logger::info(__VA_ARGS__);                                                                                 \
    } while (0)

}
