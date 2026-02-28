#pragma once

#include <ArduinoJson.h>
#include <cstdint>
#include <string>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class Ipm1CanSystem {
public:
    static Ipm1CanSystem& instance();

    bool begin();
    String getSystemJson() const;

    bool handleAction(JsonVariantConst action_json, String& error, JsonObject response);

private:
    struct Device {
        std::string id;
        std::string type;
        std::uint8_t address = 0;
    };

    struct Circuit {
        std::string name;
        std::string category;
        std::string device_id;
        bool output_is_numeric = false;
        std::int32_t output_number = -1;
        std::string output_name;
        std::vector<std::string> capabilities;
        bool user_renameable = false;
    };

    struct CircuitState {
        std::string name;
        bool is_on = false;
        std::uint8_t pwm = 0;
    };

    struct ActiveAction {
        std::string circuit;
        std::string action;
        TaskHandle_t task = nullptr;
    };

    struct ActionTaskContext {
        Ipm1CanSystem* system = nullptr;
        std::string circuit;
        std::uint8_t address = 0;
        std::int32_t output = -1;
        std::uint32_t duration_ms = 0;
        std::uint32_t period_ms = 0;
        std::uint8_t on_value = 0xFF;
        std::uint8_t off_value = 0x00;
        std::uint8_t start_pwm = 0;
        std::uint8_t target_pwm = 0;
        std::uint32_t fade_ms = 0;
    };

    static void timedTask(void* pv);
    static void flashTask(void* pv);
    static void fadeTask(void* pv);

    bool loadFromJson(const String& json, String& error);
    const Circuit* findCircuit(const std::string& name) const;
    const Device* findDevice(const std::string& id) const;
    CircuitState* findState(const std::string& name);
    bool circuitSupports(const Circuit& circuit, const std::string& action) const;

    bool applyToggle(const Circuit& circuit, const Device& device, JsonVariantConst action_json, String& error, JsonObject response);
    bool applyMomentary(const Circuit& circuit, const Device& device, JsonVariantConst action_json, String& error, JsonObject response);
    bool applyTimed(const Circuit& circuit, const Device& device, JsonVariantConst action_json, String& error, JsonObject response);
    bool applyFlash(const Circuit& circuit, const Device& device, JsonVariantConst action_json, bool timed, String& error, JsonObject response);
    bool applyFlashStop(const Circuit& circuit, const Device& device, String& error, JsonObject response);
    bool applyFade(const Circuit& circuit, const Device& device, JsonVariantConst action_json, String& error, JsonObject response);

    bool sendPowercellValue(const Device& device, std::int32_t output, std::uint8_t value, String& error);
    void cancelActiveAction(const std::string& circuit);
    void setActiveAction(const std::string& circuit, const std::string& action, TaskHandle_t task);
    void clearActiveAction(const std::string& circuit);

    String system_json_;
    std::string last_error_;
    std::vector<Device> devices_;
    std::vector<Circuit> circuits_;
    std::vector<CircuitState> states_;
    std::vector<ActiveAction> active_actions_;
};
