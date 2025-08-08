#include <windows.h>
#include <cstdio>
#include <algorithm>
#include "ConfigManager.h"

#define FOREGROUND_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)

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
    {
        WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7,
            "[Steam_API_Base] Failed to create default configuration file!\r\n");
        return false;
    }

    try
    {
        file << "; Steam API Base Configuration File\r\n";
        file << "; Generated automatically\r\n\r\n";
        file << "[steam]\r\n";
        file << "; Application ID (http://store.steampowered.com/app/%appid%/)\r\n";
        file << "appid = 480\r\n";  // Значение по умолчанию для тестирования
        file << "; Current game language\r\n";
        file << "language = english\r\n";
        file << "; Enable/disable automatic DLC unlock\r\n";
        file << "unlockall = false\r\n";
        file << "; Original Valve's steam_api.dll\r\n";
        file << "orgapi = steam_api_o.dll\r\n";
        file << "; Original Valve's steam_api64.dll\r\n";
        file << "orgapi64 = steam_api64_o.dll\r\n";
        file << "; Enable/disable extra protection bypasser\r\n";
        file << "extraprotection = false\r\n";
        file << "; The game will think that you're offline\r\n";
        file << "forceoffline = false\r\n";
        file << "; Some games are checking for the low violence presence\r\n";
        file << "lowviolence = false\r\n";
        file << "; Installation path for the game\r\n";
        file << "installdir = .\\\r\n";
        file << "; Use DLC id as the appended installation directory\r\n";
        file << "dlcasinstalldir = true\r\n";
        file << "; Purchase timestamp for the DLC\r\n";
        file << "purchasetimestamp = 0\r\n";
        file << "; Turn on the wrapper mode\r\n";
        file << "wrappermode = false\r\n\r\n";

        file << "[steam_misc]\r\n";
        file << "; Disables the internal SteamUser interface handler\r\n";
        file << "disableuserinterface = false\r\n";
        file << "; Disables the internal SteamUtils interface handler\r\n";
        file << "disableutilsinterface = false\r\n";
        file << "; Disable the internal reserve hook\r\n";
        file << "disableregisterinterfacefuncs = false\r\n";
        file << "; Unlock/Lock Steam parental restrictions\r\n";
        file << "unlockparentalrestrictions = true\r\n";
        file << "; SteamId64 to override\r\n";
        file << "steamid = 0\r\n";
        file << "; Bypass VAC signature check\r\n";
        file << "signaturebypass = false\r\n";
        file << "; Print the backtrace for the vital API functions\r\n";
        file << "printbacktrace = false\r\n\r\n";

        file << "[steam_wrapper]\r\n";
        file << "; Application ID to override (used when wrapper mode is on)\r\n";
        file << "newappid = 480\r\n";
        file << "; Use the internal storage system\r\n";
        file << "wrapperremotestorage = false\r\n";
        file << "; Use the internal stats/achievements system\r\n";
        file << "wrapperuserstats = false\r\n";
        file << "; Use the internal workshop (UGC) system\r\n";
        file << "wrapperugc = false\r\n";
        file << "; Store the data in the current directory\r\n";
        file << "saveindirectory = false\r\n";
        file << "; Force the usage of a full save path\r\n";
        file << "forcefullsavepath = false\r\n";
        file << "; Disable internal callbacks system\r\n";
        file << "disablecallbacks = false\r\n";
        file << "; Disable/Enable a StoreStats callback\r\n";
        file << "storestatscallback = true\r\n";
        file << "; Fixed achievements count\r\n";
        file << "achievementscount = 0\r\n\r\n";

        file << "[dlc]\r\n";
        file << "; DLC handling\r\n";
        file << "; Format: <dlc_id> = <dlc_name>\r\n";
        file << "; Example: 247295 = Saints Row IV - GAT V Pack\r\n\r\n";

        file << "[dlc_installdirs]\r\n";
        file << "; Installation path for the specific DLC\r\n";
        file << "; Format: <dlc_id> = <install_dir>\r\n";
        file << "; Example: 556760 = DLCRoot0\r\n\r\n";

        file << "[steam_ugc]\r\n";
        file << "; Subscribed workshop items\r\n";
        file << "; Format: <ugc_id> = <true/false>\r\n";
        file << "; Example: 812713531 = true\r\n\r\n";

        file.close();

        WriteColoredText(FOREGROUND_GREEN | FOREGROUND_INTENSITY, 7,
            "[Steam_API_Base] Default configuration file created successfully!\r\n");

        return true;
    }
    catch (const std::exception& e)
    {
        char buffer[1024];
        sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Exception while creating default config: %s\r\n", e.what());
        WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, buffer);
        return false;
    }
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

    // Очищаем существующие секции перед загрузкой новых
    m_ConfigSections.clear();

    std::string line;
    std::string currentSection;
    int lineNumber = 0;

    while (std::getline(file, line))
    {
        lineNumber++;
        line = Trim(line);

        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == ';' || line[0] == '#')
        {
            continue;
        }

        // Проверяем, является ли строка секцией
        if (line[0] == '[' && line[line.length() - 1] == ']')
        {
            currentSection = line.substr(1, line.length() - 2);
            // Проверяем, что имя секции не пустое
            if (currentSection.empty())
            {
                char buffer[1024];
                sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Empty section name at line %d\r\n", lineNumber);
                WriteColoredText(FOREGROUND_YELLOW | FOREGROUND_INTENSITY, 7, buffer);
                continue;
            }
            continue;
        }

        // Если секция не установлена, пропускаем
        if (currentSection.empty())
        {
            char buffer[1024];
            sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Key-value pair without section at line %d\r\n", lineNumber);
            WriteColoredText(FOREGROUND_YELLOW | FOREGROUND_INTENSITY, 7, buffer);
            continue;
        }

        // Разделяем ключ и значение
        size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos)
        {
            char buffer[1024];
            sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Invalid key-value format at line %d: %s\r\n", lineNumber, line.c_str());
            WriteColoredText(FOREGROUND_YELLOW | FOREGROUND_INTENSITY, 7, buffer);
            continue;
        }

        std::string key = Trim(line.substr(0, delimiterPos));
        std::string value = Trim(line.substr(delimiterPos + 1));

        if (key.empty())
        {
            char buffer[1024];
            sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Empty key at line %d\r\n", lineNumber);
            WriteColoredText(FOREGROUND_YELLOW | FOREGROUND_INTENSITY, 7, buffer);
            continue;
        }

        // Добавляем в конфигурацию - БЕЗОПАСНЫЙ СПОСОБ
        // Проверяем, существует ли секция
        auto sectionIt = m_ConfigSections.find(currentSection);
        if (sectionIt == m_ConfigSections.end())
        {
            // Секция не существует, создаем ее
            m_ConfigSections[currentSection] = std::map<std::string, std::string>();
            sectionIt = m_ConfigSections.find(currentSection);
        }

        // Теперь безопасно добавляем ключ-значение
        if (sectionIt != m_ConfigSections.end())
        {
            sectionIt->second[key] = value;
        }
    }

    file.close();

    char buffer[1024];
    sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Configuration loaded successfully from: %s\r\n", configPath.c_str());
    WriteColoredText(FOREGROUND_GREEN | FOREGROUND_INTENSITY, 7, buffer);

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
    // Проверяем, существует ли секция
    auto sectionIt = m_ConfigSections.find(section);
    if (sectionIt == m_ConfigSections.end())
    {
        return defaultValue;
    }

    // Проверяем, существует ли ключ в секции
    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end())
    {
        return defaultValue;
    }

    return keyIt->second;
}

int CConfigManager::GetInt(const std::string& section, const std::string& key, int defaultValue)
{
    std::string value = GetString(section, key, "");
    if (value.empty())
    {
        return defaultValue;
    }

    try
    {
        return std::stoi(value);
    }
    catch (...)
    {
        return defaultValue;
    }
}

bool CConfigManager::GetBool(const std::string& section, const std::string& key, bool defaultValue)
{
    std::string value = GetString(section, key, "");
    if (value.empty())
    {
        return defaultValue;
    }

    // Преобразуем в нижний регистр для сравнения
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);

    return (value == "true" || value == "1" || value == "yes" || value == "on");
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

    // Проверяем, существует ли секция [dlc]
    auto sectionIt = m_ConfigSections.find("dlc");
    if (sectionIt == m_ConfigSections.end())
    {
        return dlcList; // Секция не найдена, возвращаем пустой список
    }

    // Безопасно обрабатываем все элементы секции
    for (const auto& keyValue : sectionIt->second)
    {
        try
        {
            uint32 dlcId = std::stoul(keyValue.first);
            std::string dlcName = keyValue.second;
            dlcList.emplace_back(dlcId, dlcName);
        }
        catch (...)
        {
            char buffer[1024];
            sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Invalid DLC ID format: %s\r\n", keyValue.first.c_str());
            WriteColoredText(FOREGROUND_YELLOW | FOREGROUND_INTENSITY, 7, buffer);
        }
    }

    return dlcList;
}

std::map<uint32, std::string> CConfigManager::GetDLCInstallDirs()
{
    std::map<uint32, std::string> installDirs;

    // Проверяем, существует ли секция [dlc_installdirs]
    auto sectionIt = m_ConfigSections.find("dlc_installdirs");
    if (sectionIt == m_ConfigSections.end())
    {
        return installDirs; // Секция не найдена, возвращаем пустую карту
    }

    // Безопасно обрабатываем все элементы секции
    for (const auto& keyValue : sectionIt->second)
    {
        try
        {
            uint32 dlcId = std::stoul(keyValue.first);
            std::string installDir = keyValue.second;
            installDirs[dlcId] = installDir;
        }
        catch (...)
        {
            char buffer[1024];
            sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Invalid DLC install dir ID format: %s\r\n", keyValue.first.c_str());
            WriteColoredText(FOREGROUND_YELLOW | FOREGROUND_INTENSITY, 7, buffer);
        }
    }

    return installDirs;
}

std::map<uint32, bool> CConfigManager::GetUGCItems()
{
    std::map<uint32, bool> ugcItems;

    // Проверяем, существует ли секция [steam_ugc]
    auto sectionIt = m_ConfigSections.find("steam_ugc");
    if (sectionIt == m_ConfigSections.end())
    {
        return ugcItems; // Секция не найдена, возвращаем пустую карту
    }

    // Безопасно обрабатываем все элементы секции
    for (const auto& keyValue : sectionIt->second)
    {
        try
        {
            uint32 ugcId = std::stoul(keyValue.first);
            std::string value = keyValue.second;

            // Преобразуем строку в нижний регистр для сравнения
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);

            // Определяем значение true/false
            bool isEnabled = (value == "true" || value == "1" || value == "yes" || value == "on");
            ugcItems[ugcId] = isEnabled;
        }
        catch (...)
        {
            char buffer[1024];
            sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Invalid UGC ID format: %s\r\n", keyValue.first.c_str());
            WriteColoredText(FOREGROUND_YELLOW | FOREGROUND_INTENSITY, 7, buffer);
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
    // Проверяем, существует ли секция
    auto sectionIt = m_ConfigSections.find(section);
    if (sectionIt == m_ConfigSections.end())
    {
        return false; // Секция не найдена
    }

    // Проверяем, существует ли ключ в секции
    auto keyIt = sectionIt->second.find(key);
    return (keyIt != sectionIt->second.end());
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
