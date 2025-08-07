#include <windows.h>
#include <cstdio>
#include <algorithm>
#include "ConfigManager.h"

// Внешняя функция для вывода цветного текста
void WriteColoredText(WORD Color, WORD OriginalColor, const char* Text);

CConfigManager::CConfigManager()
{
    m_ConfigFilePath = "steam_api_config.ini";
}

CConfigManager::~CConfigManager()
{
}

std::string CConfigManager::Trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool CConfigManager::CreateDefaultConfig()
{
    std::ofstream file(m_ConfigFilePath);
    if (!file.is_open())
        return false;

    file << "; Steam API Base Configuration File\n";
    file << "; Generated automatically\n\n";

    file << "[steam]\n";
    file << "; Application ID (http://store.steampowered.com/app/%appid%/)\n";
    file << "appid = 480\n";
    file << "; Current game language\n";
    file << "language = english\n";
    file << "; Enable/disable automatic DLC unlock\n";
    file << "unlockall = false\n";
    file << "; Original Valve's steam_api.dll\n";
    file << "orgapi = steam_api_o.dll\n";
    file << "; Original Valve's steam_api64.dll\n";
    file << "orgapi64 = steam_api64_o.dll\n";
    file << "; Enable/disable extra protection bypasser\n";
    file << "extraprotection = false\n";
    file << "; The game will think that you're offline\n";
    file << "forceoffline = false\n";
    file << "; Some games are checking for the low violence presence\n";
    file << "lowviolence = false\n";
    file << "; Installation path for the game\n";
    file << "installdir = .\\\n";
    file << "; Use DLC id as the appended installation directory\n";
    file << "dlcasinstalldir = true\n";
    file << "; Purchase timestamp for the DLC\n";
    file << "purchasetimestamp = 0\n";
    file << "; Turn on the wrapper mode\n";
    file << "wrappermode = false\n\n";

    file << "[steam_misc]\n";
    file << "; Disables the internal SteamUser interface handler\n";
    file << "disableuserinterface = false\n";
    file << "; Disables the internal SteamUtils interface handler\n";
    file << "disableutilsinterface = false\n";
    file << "; Disable the internal reserve hook\n";
    file << "disableregisterinterfacefuncs = false\n";
    file << "; Unlock/Lock Steam parental restrictions\n";
    file << "unlockparentalrestrictions = true\n";
    file << "; SteamId64 to override\n";
    file << "steamid = 0\n";
    file << "; Bypass VAC signature check\n";
    file << "signaturebypass = false\n";
    file << "; Print the backtrace for the vital API functions\n";
    file << "printbacktrace = false\n\n";

    file << "[steam_wrapper]\n";
    file << "; Application ID to override (used when wrapper mode is on)\n";
    file << "newappid = 480\n";
    file << "; Use the internal storage system\n";
    file << "wrapperremotestorage = false\n";
    file << "; Use the internal stats/achievements system\n";
    file << "wrapperuserstats = false\n";
    file << "; Use the internal workshop (UGC) system\n";
    file << "wrapperugc = false\n";
    file << "; Store the data in the current directory\n";
    file << "saveindirectory = false\n";
    file << "; Force the usage of a full save path\n";
    file << "forcefullsavepath = false\n";
    file << "; Disable internal callbacks system\n";
    file << "disablecallbacks = false\n";
    file << "; Disable/Enable a StoreStats callback\n";
    file << "storestatscallback = true\n";
    file << "; Fixed achievements count\n";
    file << "achievementscount = 0\n\n";

    file << "[dlc]\n";
    file << "; DLC handling\n";
    file << "; Format: <dlc_id> = <dlc_description>\n";
    file << "; Example: 247295 = Saints Row IV - GAT V Pack\n\n";

    file << "[dlc_installdirs]\n";
    file << "; Installation path for the specific DLC\n";
    file << "; Format: <dlc_id> = <install_dir>\n";
    file << "; Example: 556760 = DLCRoot0\n\n";

    file << "[steam_ugc]\n";
    file << "; Subscribed workshop items\n";
    file << "; Format: <ugc_id> = <true/false>\n";
    file << "; Example: 812713531 = true\n\n";

    file.close();

    WriteColoredText(FOREGROUND_GREEN | FOREGROUND_INTENSITY, 7,
        "[Steam_API_Base] Default configuration file created!\r\n");

    return true;
}

bool CConfigManager::LoadConfig(const std::string& configPath)
{
    m_ConfigFilePath = configPath;
    std::ifstream file(configPath);

    if (!file.is_open())
    {
        WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7,
            "[Steam_API_Base] Configuration file not found, creating default...\r\n");
        return CreateDefaultConfig();
    }

    m_ConfigSections.clear();

    std::string line;
    std::string currentSection;

    while (std::getline(file, line))
    {
        line = Trim(line);

        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;

        // Проверяем секции
        if (line[0] == '[' && line.back() == ']')
        {
            currentSection = line.substr(1, line.length() - 2);
            currentSection = Trim(currentSection);
            continue;
        }

        // Парсим ключ=значение
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos && !currentSection.empty())
        {
            std::string key = Trim(line.substr(0, equalPos));
            std::string value = Trim(line.substr(equalPos + 1));

            m_ConfigSections[currentSection][key] = value;
        }
    }

    file.close();

    WriteColoredText(FOREGROUND_GREEN | FOREGROUND_INTENSITY, 7,
        "[Steam_API_Base] Configuration loaded successfully!\r\n");

    return true;
}

bool CConfigManager::SaveConfig()
{
    std::ofstream file(m_ConfigFilePath);
    if (!file.is_open())
        return false;

    for (const auto& section : m_ConfigSections)
    {
        file << "[" << section.first << "]\n";
        for (const auto& keyValue : section.second)
        {
            file << keyValue.first << " = " << keyValue.second << "\n";
        }
        file << "\n";
    }

    file.close();
    return true;
}

std::string CConfigManager::GetString(const std::string& section, const std::string& key, const std::string& defaultValue)
{
    auto sectionIt = m_ConfigSections.find(section);
    if (sectionIt != m_ConfigSections.end())
    {
        auto keyIt = sectionIt->second.find(key);
        if (keyIt != sectionIt->second.end())
        {
            return keyIt->second;
        }
    }
    return defaultValue;
}

int CConfigManager::GetInt(const std::string& section, const std::string& key, int defaultValue)
{
    std::string value = GetString(section, key);
    if (!value.empty())
    {
        try
        {
            return std::stoi(value);
        }
        catch (...)
        {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool CConfigManager::GetBool(const std::string& section, const std::string& key, bool defaultValue)
{
    std::string value = GetString(section, key);
    if (!value.empty())
    {
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return (value == "true" || value == "1" || value == "yes");
    }
    return defaultValue;
}

void CConfigManager::SetString(const std::string& section, const std::string& key, const std::string& value)
{
    m_ConfigSections[section][key] = value;
}

void CConfigManager::SetInt(const std::string& section, const std::string& key, int value)
{
    m_ConfigSections[section][key] = std::to_string(value);
}

void CConfigManager::SetBool(const std::string& section, const std::string& key, bool value)
{
    m_ConfigSections[section][key] = value ? "true" : "false";
}

std::vector<std::pair<uint32, std::string>> CConfigManager::GetDLCList()
{
    std::vector<std::pair<uint32, std::string>> dlcList;

    auto sectionIt = m_ConfigSections.find("dlc");
    if (sectionIt != m_ConfigSections.end())
    {
        for (const auto& keyValue : sectionIt->second)
        {
            try
            {
                uint32 dlcId = std::stoul(keyValue.first);
                dlcList.push_back(std::make_pair(dlcId, keyValue.second));
            }
            catch (...)
            {
                // Пропускаем некорректные ID
            }
        }
    }

    return dlcList;
}

std::map<uint32, std::string> CConfigManager::GetDLCInstallDirs()
{
    std::map<uint32, std::string> installDirs;

    auto sectionIt = m_ConfigSections.find("dlc_installdirs");
    if (sectionIt != m_ConfigSections.end())
    {
        for (const auto& keyValue : sectionIt->second)
        {
            try
            {
                uint32 dlcId = std::stoul(keyValue.first);
                installDirs[dlcId] = keyValue.second;
            }
            catch (...)
            {
                // Пропускаем некорректные ID
            }
        }
    }

    return installDirs;
}

std::map<uint32, bool> CConfigManager::GetUGCItems()
{
    std::map<uint32, bool> ugcItems;

    auto sectionIt = m_ConfigSections.find("steam_ugc");
    if (sectionIt != m_ConfigSections.end())
    {
        for (const auto& keyValue : sectionIt->second)
        {
            try
            {
                uint32 ugcId = std::stoul(keyValue.first);
                std::string value = keyValue.second;
                std::transform(value.begin(), value.end(), value.begin(), ::tolower);
                ugcItems[ugcId] = (value == "true" || value == "1");
            }
            catch (...)
            {
                // Пропускаем некорректные ID
            }
        }
    }

    return ugcItems;
}

bool CConfigManager::HasSection(const std::string& section)
{
    return m_ConfigSections.find(section) != m_ConfigSections.end();
}

bool CConfigManager::HasKey(const std::string& section, const std::string& key)
{
    auto sectionIt = m_ConfigSections.find(section);
    if (sectionIt != m_ConfigSections.end())
    {
        return sectionIt->second.find(key) != sectionIt->second.end();
    }
    return false;
}

void CConfigManager::PrintConfig()
{
    for (const auto& section : m_ConfigSections)
    {
        printf("[Steam_API_Base] Section: [%s]\r\n", section.first.c_str());
        for (const auto& keyValue : section.second)
        {
            printf("[Steam_API_Base]   %s = %s\r\n", keyValue.first.c_str(), keyValue.second.c_str());
        }
    }
}

CConfigManager* GConfigManager()
{
    static CConfigManager ConfigManager;
    return &ConfigManager;
}
