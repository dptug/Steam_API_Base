#include <cstdio>
#include "SteamSettings.h"

// Внешняя функция для вывода цветного текста
void WriteColoredText(WORD Color, WORD OriginalColor, const char* Text);

CSteamSettings::CSteamSettings()
{
    // Конструкторы структур уже инициализируют значения по умолчанию
}

CSteamSettings::~CSteamSettings()
{
}

bool CSteamSettings::LoadSettings()
{
    CConfigManager* config = GConfigManager();

    // Загружаем основные настройки Steam
    m_SteamConfig.appId = config->GetInt("steam", "appid", 480);
    m_SteamConfig.language = config->GetString("steam", "language", "english");
    m_SteamConfig.unlockAll = config->GetBool("steam", "unlockall", false);
    m_SteamConfig.orgApi = config->GetString("steam", "orgapi", "steam_api_o.dll");
    m_SteamConfig.orgApi64 = config->GetString("steam", "orgapi64", "steam_api64_o.dll");
    m_SteamConfig.extraProtection = config->GetBool("steam", "extraprotection", false);
    m_SteamConfig.forceOffline = config->GetBool("steam", "forceoffline", false);
    m_SteamConfig.lowViolence = config->GetBool("steam", "lowviolence", false);
    m_SteamConfig.installDir = config->GetString("steam", "installdir", ".\\");
    m_SteamConfig.dlcAsInstallDir = config->GetBool("steam", "dlcasinstalldir", true);
    m_SteamConfig.purchaseTimestamp = config->GetInt("steam", "purchasetimestamp", 0);
    m_SteamConfig.wrapperMode = config->GetBool("steam", "wrappermode", false);

    // Загружаем настройки steam_misc
    m_MiscConfig.disableUserInterface = config->GetBool("steam_misc", "disableuserinterface", false);
    m_MiscConfig.disableUtilsInterface = config->GetBool("steam_misc", "disableutilsinterface", false);
    m_MiscConfig.disableRegisterInterfaceFuncs = config->GetBool("steam_misc", "disableregisterinterfacefuncs", false);
    m_MiscConfig.unlockParentalRestrictions = config->GetBool("steam_misc", "unlockparentalrestrictions", true);
    m_MiscConfig.steamId = config->GetInt("steam_misc", "steamid", 0);
    m_MiscConfig.signatureBypass = config->GetBool("steam_misc", "signaturebypass", false);
    m_MiscConfig.printBacktrace = config->GetBool("steam_misc", "printbacktrace", false);

    // Загружаем настройки steam_wrapper
    m_WrapperConfig.newAppId = config->GetInt("steam_wrapper", "newappid", 480);
    m_WrapperConfig.wrapperRemoteStorage = config->GetBool("steam_wrapper", "wrapperremotestorage", false);
    m_WrapperConfig.wrapperUserStats = config->GetBool("steam_wrapper", "wrapperuserstats", false);
    m_WrapperConfig.wrapperUgc = config->GetBool("steam_wrapper", "wrapperugc", false);
    m_WrapperConfig.saveInDirectory = config->GetBool("steam_wrapper", "saveindirectory", false);
    m_WrapperConfig.forceFulSavePath = config->GetBool("steam_wrapper", "forcefullsavepath", false);
    m_WrapperConfig.disableCallbacks = config->GetBool("steam_wrapper", "disablecallbacks", false);
    m_WrapperConfig.storeStatsCallback = config->GetBool("steam_wrapper", "storestatscallback", true);
    m_WrapperConfig.achievementsCount = config->GetInt("steam_wrapper", "achievementscount", 0);

    // Загружаем списки DLC
    auto dlcList = config->GetDLCList();
    m_DLCList.clear();
    for (const auto& dlc : dlcList)
    {
        m_DLCList[dlc.first] = dlc.second;
    }

    // Загружаем пути установки DLC
    m_DLCInstallDirs = config->GetDLCInstallDirs();

    // Загружаем UGC элементы
    m_UGCItems = config->GetUGCItems();

    WriteColoredText(FOREGROUND_GREEN | FOREGROUND_INTENSITY, 7,
        "[Steam_API_Base] Steam settings loaded successfully!\r\n");

    return true;
}

bool CSteamSettings::SaveSettings()
{
    CConfigManager* config = GConfigManager();

    // Сохраняем основные настройки Steam
    config->SetInt("steam", "appid", m_SteamConfig.appId);
    config->SetString("steam", "language", m_SteamConfig.language);
    config->SetBool("steam", "unlockall", m_SteamConfig.unlockAll);
    config->SetString("steam", "orgapi", m_SteamConfig.orgApi);
    config->SetString("steam", "orgapi64", m_SteamConfig.orgApi64);
    config->SetBool("steam", "extraprotection", m_SteamConfig.extraProtection);
    config->SetBool("steam", "forceoffline", m_SteamConfig.forceOffline);
    config->SetBool("steam", "lowviolence", m_SteamConfig.lowViolence);
    config->SetString("steam", "installdir", m_SteamConfig.installDir);
    config->SetBool("steam", "dlcasinstalldir", m_SteamConfig.dlcAsInstallDir);
    config->SetInt("steam", "purchasetimestamp", m_SteamConfig.purchaseTimestamp);
    config->SetBool("steam", "wrappermode", m_SteamConfig.wrapperMode);

    // Сохраняем остальные секции...
    // (аналогично LoadSettings, но с вызовами Set вместо Get)

    return config->SaveConfig();
}

bool CSteamSettings::IsDLCUnlocked(uint32 dlcId) const
{
    if (m_SteamConfig.unlockAll)
        return true;

    return m_DLCList.find(dlcId) != m_DLCList.end();
}

bool CSteamSettings::IsUGCSubscribed(uint32 ugcId) const
{
    auto it = m_UGCItems.find(ugcId);
    return (it != m_UGCItems.end()) ? it->second : false;
}

std::string CSteamSettings::GetDLCInstallPath(uint32 dlcId) const
{
    auto it = m_DLCInstallDirs.find(dlcId);
    if (it != m_DLCInstallDirs.end())
        return it->second;

    if (m_SteamConfig.dlcAsInstallDir)
        return m_SteamConfig.installDir + std::to_string(dlcId);

    return m_SteamConfig.installDir;
}

uint32 CSteamSettings::GetActiveAppId() const
{
    if (m_SteamConfig.wrapperMode)
        return m_WrapperConfig.newAppId;

    return m_SteamConfig.appId;
}

std::string CSteamSettings::GetGameLanguage() const
{
    return m_SteamConfig.language;
}

void CSteamSettings::PrintSettings() const
{
    printf("[Steam_API_Base] === Steam Settings ===\r\n");
    printf("[Steam_API_Base] App ID: %u\r\n", GetActiveAppId());
    printf("[Steam_API_Base] Language: %s\r\n", m_SteamConfig.language.c_str());
    printf("[Steam_API_Base] Wrapper Mode: %s\r\n", m_SteamConfig.wrapperMode ? "true" : "false");
    printf("[Steam_API_Base] Unlock All DLC: %s\r\n", m_SteamConfig.unlockAll ? "true" : "false");
    printf("[Steam_API_Base] DLC Count: %zu\r\n", m_DLCList.size());
    printf("[Steam_API_Base] UGC Items: %zu\r\n", m_UGCItems.size());
}

CSteamSettings* GSteamSettings()
{
    static CSteamSettings SteamSettings;
    return &SteamSettings;
}
