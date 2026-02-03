#pragma once

#include <ArduinoJson.h>
#include <string>

#include "config_types.h"

class ConfigManager {
public:
    static ConfigManager& instance();

    bool begin();
    bool save() const;
    bool resetToDefaults();
    void factoryReset();  // Nuclear option - wipe config file

    DeviceConfig& getConfig() { return config_; }
    const DeviceConfig& getConfig() const { return config_; }

    std::string toJson() const;
    bool updateFromJson(JsonVariantConst json, std::string& error);

private:
    ConfigManager() = default;

    DeviceConfig config_{};

    bool loadFromStorage();
    bool writeToStorage(const std::string& json) const;
    DeviceConfig buildDefaultConfig() const;
    static int compareVersions(const std::string& lhs, const std::string& rhs);
    bool decodeConfig(JsonVariantConst json, DeviceConfig& target, std::string& error) const;
    void encodeConfig(const DeviceConfig& source, DynamicJsonDocument& doc) const;
};
