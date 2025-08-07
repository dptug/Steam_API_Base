#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <cstdint>  // Add this include
typedef uint32_t uint32;  // Add this typedef

#pragma once
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>

class CConfigManager
{
private:
    std::map<std::string, std::map<std::string, std::string>> m_ConfigSections;
    std::string m_ConfigFilePath;

    bool CreateDefaultConfig(); // Добавляем объявление функции
    std::string Trim(const std::string& str);

public:
    CConfigManager();
    ~CConfigManager();

    bool LoadConfig(const std::string& configPath = "steam_api_config.ini");
    bool SaveConfig();

    std::string GetString(const std::string& section, const std::string& key, const std::string& defaultValue = "");
    int GetInt(const std::string& section, const std::string& key, int defaultValue = 0);
    bool GetBool(const std::string& section, const std::string& key, bool defaultValue = false);

    void SetString(const std::string& section, const std::string& key, const std::string& value);
    void SetInt(const std::string& section, const std::string& key, int value);
    void SetBool(const std::string& section, const std::string& key, bool value);

    std::vector<std::pair<uint32, std::string>> GetDLCList();
    std::map<uint32, std::string> GetDLCInstallDirs();
    std::map<uint32, bool> GetUGCItems();

    bool HasSection(const std::string& section);
    bool HasKey(const std::string& section, const std::string& key);

    void PrintConfig(); // Для отладки
};

extern CConfigManager* GConfigManager();

#endif // CONFIGMANAGER_H
