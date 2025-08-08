#ifndef STEAMSETINGS_H
#define STEAMSETINGS_H

#pragma once
#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include "ConfigManager.h"

typedef unsigned int uint32;
typedef unsigned long long uint64;

class CSteamSettings
{
public:
    struct SteamConfig
    {
        uint32 appId;
        std::string language;
        bool unlockAll;
        std::string orgApi;
        std::string orgApi64;
        bool extraProtection;
        bool forceOffline;
        bool lowViolence;
        std::string installDir;
        bool dlcAsInstallDir;
        uint32 purchaseTimestamp;
        bool wrapperMode;

        SteamConfig() :
            appId(480), language("english"), unlockAll(false),
            orgApi("steam_api_o.dll"), orgApi64("steam_api64_o.dll"),
            extraProtection(false), forceOffline(false), lowViolence(false),
            installDir(".\\"), dlcAsInstallDir(true), purchaseTimestamp(0),
            wrapperMode(false) {
        }
    };

    struct SteamMiscConfig
    {
        bool disableUserInterface;
        bool disableUtilsInterface;
        bool disableRegisterInterfaceFuncs;
        bool unlockParentalRestrictions;
        uint64 steamId;
        bool signatureBypass;
        bool printBacktrace;

        SteamMiscConfig() :
            disableUserInterface(false), disableUtilsInterface(false),
            disableRegisterInterfaceFuncs(false), unlockParentalRestrictions(true),
            steamId(0), signatureBypass(false), printBacktrace(false) {
        }
    };

    struct SteamWrapperConfig
    {
        uint32 newAppId;
        bool wrapperRemoteStorage;
        bool wrapperUserStats;
        bool wrapperUgc;
        bool saveInDirectory;
        bool forceFulSavePath;
        bool disableCallbacks;
        bool storeStatsCallback;
        uint32 achievementsCount;

        SteamWrapperConfig() :
            newAppId(480), wrapperRemoteStorage(false), wrapperUserStats(false),
            wrapperUgc(false), saveInDirectory(false), forceFulSavePath(false),
            disableCallbacks(false), storeStatsCallback(true), achievementsCount(0) {
        }
    };

private:
    SteamConfig m_SteamConfig;
    SteamMiscConfig m_MiscConfig;
    SteamWrapperConfig m_WrapperConfig;
    std::map<uint32, std::string> m_DLCList;
    std::map<uint32, std::string> m_DLCInstallDirs;
    std::map<uint32, bool> m_UGCItems;

public:
    CSteamSettings();
    ~CSteamSettings();

    bool LoadSettings();
    bool SaveSettings();

    const SteamConfig& GetSteamConfig() const { return m_SteamConfig; }
    const SteamMiscConfig& GetMiscConfig() const { return m_MiscConfig; }
    const SteamWrapperConfig& GetWrapperConfig() const { return m_WrapperConfig; }

    const std::map<uint32, std::string>& GetDLCList() const { return m_DLCList; }
    const std::map<uint32, std::string>& GetDLCInstallDirs() const { return m_DLCInstallDirs; }
    const std::map<uint32, bool>& GetUGCItems() const { return m_UGCItems; }

    bool IsDLCUnlocked(uint32 dlcId) const;
    bool IsUGCSubscribed(uint32 ugcId) const;
    std::string GetDLCInstallPath(uint32 dlcId) const;

    uint32 GetActiveAppId() const;
    std::string GetGameLanguage() const;

    void PrintSettings() const; // Для отладки

    // Получить оригинальный AppID (до подмены)
    uint32 GetOriginalAppId() const { return m_SteamConfig.appId; }

    // Получить AppID для подмены (только в режиме обертки)
    uint32 GetWrapperAppId() const { return m_WrapperConfig.newAppId; }

    // Проверить, включен ли режим обертки
    bool IsWrapperMode() const { return m_SteamConfig.wrapperMode; }
};

extern CSteamSettings* GSteamSettings();

#endif // STEAMSETINGS_H
