#include "ipm1_can_system.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "can_manager.h"
#include "ipm1_can_library.h"

namespace {
constexpr const char* kSystemPath = "/ipm1_can_system_full.json";

constexpr const char* kDefaultSystemJson = R"json(
{
  "meta": {
    "system": "Infinitybox IPM1",
    "revision": "REV1",
    "source": "Front Engine Standard System Assignments",
    "purpose": "Complete UI + CAN behavioral model"
  },

  "devices": [
    { "id": "pc_front", "type": "powercell", "address": 1 },
    { "id": "pc_rear", "type": "powercell", "address": 2 },
    { "id": "im_df", "type": "inmotion", "address": 3 },
    { "id": "im_pf", "type": "inmotion", "address": 4 },
    { "id": "im_dr", "type": "inmotion", "address": 5 },
    { "id": "im_pr", "type": "inmotion", "address": 6 },
    { "id": "mastercell", "type": "mastercell" }
  ],

  "functions": [
    { "name": "Left Turn Signal Front", "device": "pc_front", "output": 1, "behaviors": ["flash","flash_timed"], "requires": ["ignition"] },
    { "name": "Right Turn Signal Front", "device": "pc_front", "output": 2, "behaviors": ["flash","flash_timed"], "requires": ["ignition"] },
    { "name": "4-Ways", "device": "pc_front", "outputs": [1,2], "behaviors": ["flash"] },
    { "name": "Ignition", "device": "pc_front", "output": 3, "behaviors": ["toggle"] },
    { "name": "Starter", "device": "pc_front", "output": 4, "behaviors": ["momentary"], "blocked_when": ["security"] },
    { "name": "Headlights", "device": "pc_front", "output": 5, "behaviors": ["toggle","scene","fade"] },
    { "name": "Parking Lights Front", "device": "pc_front", "output": 6, "behaviors": ["toggle"] },
    { "name": "High Beams", "device": "pc_front", "output": 7, "behaviors": ["momentary","toggle"] },
    { "name": "Horn", "device": "pc_front", "output": 9, "behaviors": ["momentary"] },
    { "name": "Cooling Fan", "device": "pc_front", "output": 10, "behaviors": ["toggle","timed"] },
    { "name": "Left Turn Signal Rear", "device": "pc_rear", "output": 1, "behaviors": ["flash","flash_timed"] },
    { "name": "Right Turn Signal Rear", "device": "pc_rear", "output": 2, "behaviors": ["flash","flash_timed"] },
    { "name": "Brake Lights", "device": "pc_rear", "output": 3, "behaviors": ["toggle"] },
    { "name": "Interior Lights", "device": "pc_rear", "output": 4, "behaviors": ["toggle","fade","timed"] },
    { "name": "Backup Lights", "device": "pc_rear", "output": 5, "behaviors": ["toggle"] },
    { "name": "Parking Lights Rear", "device": "pc_rear", "output": 6, "behaviors": ["toggle"] },
    { "name": "Fuel Pump", "device": "pc_rear", "output": 10, "behaviors": ["toggle"], "blocked_when": ["security"] },
    { "name": "Driver Window Up", "device": "im_df", "output": "relay_1a", "behaviors": ["momentary"] },
    { "name": "Driver Window Down", "device": "im_df", "output": "relay_1b", "behaviors": ["momentary"] },
    { "name": "Driver Door Lock", "device": "im_df", "output": "relay_2a", "behaviors": ["one_shot"] },
    { "name": "Driver Door Unlock", "device": "im_df", "output": "relay_2b", "behaviors": ["one_shot"] },
    { "name": "Passenger Window Up", "device": "im_pf", "output": "relay_1a", "behaviors": ["momentary"] },
    { "name": "Passenger Window Down", "device": "im_pf", "output": "relay_1b", "behaviors": ["momentary"] },
    { "name": "AUX 03", "device": "im_df", "output": "aux_03", "behaviors": ["toggle","flash","fade","timed"], "renameable": true },
    { "name": "AUX 04", "device": "im_df", "output": "aux_04", "behaviors": ["toggle","flash","fade","timed"], "renameable": true }
  ]
}
)json";

bool parseBool(JsonVariantConst value, bool& out) {
    if (value.is<bool>()) {
        out = value.as<bool>();
        return true;
    }
    if (value.is<int>()) {
        out = value.as<int>() != 0;
        return true;
    }
    if (value.is<const char*>()) {
        String raw = value.as<const char*>();
        raw.toLowerCase();
        if (raw == "on" || raw == "true" || raw == "1") {
            out = true;
            return true;
        }
        if (raw == "off" || raw == "false" || raw == "0") {
            out = false;
            return true;
        }
    }
    return false;
}
}  // namespace

Ipm1CanSystem& Ipm1CanSystem::instance() {
    static Ipm1CanSystem system;
    return system;
}

bool Ipm1CanSystem::begin() {
    String error;
    // Try to load persisted system JSON from LittleFS first
    if (LittleFS.exists(kSystemPath)) {
        File file = LittleFS.open(kSystemPath, "r");
        if (file) {
            system_json_ = file.readString();
            file.close();
        }
    }

    // If no file exists or it couldn't be read, fall back to the
    // built-in default JSON and persist it so future boots are clean.
    if (system_json_.isEmpty()) {
        Serial.println("[IPM1] No system JSON found on LittleFS, writing defaults to /ipm1_can_system_full.json");
        system_json_ = kDefaultSystemJson;

        File file = LittleFS.open(kSystemPath, "w");
        if (file) {
            file.print(system_json_);
            file.close();
            Serial.println("[IPM1] Default system JSON written successfully");
        } else {
            Serial.println("[IPM1] WARNING: Failed to open /ipm1_can_system_full.json for write");
        }
    }

    if (!loadFromJson(system_json_, error)) {
        last_error_ = error.c_str();
        return false;
    }

    return true;
}

String Ipm1CanSystem::getSystemJson() const {
    if (system_json_.isEmpty()) {
        return String(kDefaultSystemJson);
    }
    return system_json_;
}

bool Ipm1CanSystem::loadFromJson(const String& json, String& error) {
    devices_.clear();
    circuits_.clear();
    states_.clear();

    DynamicJsonDocument doc(16384);
    auto result = deserializeJson(doc, json);
    if (result != DeserializationError::Ok) {
        error = String("Failed to parse IPM1 system JSON: ") + result.c_str();
        return false;
    }

    JsonArray devices = doc["devices"].as<JsonArray>();
    for (JsonObject dev : devices) {
        Device device;
        device.id = dev["id"] | "";
        device.type = dev["type"] | "";
        device.address = dev["address"] | 0;
        if (!device.id.empty()) {
            devices_.push_back(device);
        }
    }

    JsonArray functions = doc["functions"].as<JsonArray>();
    for (JsonObject f : functions) {
        Circuit circuit;
        circuit.name = f["name"] | "";
        circuit.category = "";
        circuit.device_id = f["device"] | "";
        circuit.user_renameable = f["renameable"] | false;

        JsonVariant output = f["output"];
        if (output.is<int>()) {
            circuit.output_is_numeric = true;
            circuit.output_number = output.as<int>();
        } else if (output.is<const char*>()) {
            circuit.output_is_numeric = false;
            circuit.output_name = output.as<const char*>();
        }

        JsonArray behaviors = f["behaviors"].as<JsonArray>();
        for (JsonVariant behavior : behaviors) {
            if (behavior.is<const char*>()) {
                circuit.capabilities.emplace_back(behavior.as<const char*>());
            }
        }

        if (!circuit.name.empty()) {
            circuits_.push_back(circuit);
            CircuitState state;
            state.name = circuit.name;
            states_.push_back(state);
        }
    }

    return true;
}

const Ipm1CanSystem::Circuit* Ipm1CanSystem::findCircuit(const std::string& name) const {
    for (const auto& circuit : circuits_) {
        if (circuit.name == name) {
            return &circuit;
        }
    }
    return nullptr;
}

const Ipm1CanSystem::Device* Ipm1CanSystem::findDevice(const std::string& id) const {
    for (const auto& device : devices_) {
        if (device.id == id) {
            return &device;
        }
    }
    return nullptr;
}

Ipm1CanSystem::CircuitState* Ipm1CanSystem::findState(const std::string& name) {
    for (auto& state : states_) {
        if (state.name == name) {
            return &state;
        }
    }
    return nullptr;
}

bool Ipm1CanSystem::circuitSupports(const Circuit& circuit, const std::string& action) const {
    for (const auto& cap : circuit.capabilities) {
        if (cap == action) {
            return true;
        }
    }
    return false;
}

bool Ipm1CanSystem::handleAction(JsonVariantConst action_json, String& error, JsonObject response) {
    if (!action_json.is<JsonObjectConst>()) {
        error = "Action payload must be a JSON object";
        return false;
    }

    JsonObjectConst obj = action_json.as<JsonObjectConst>();
    String action = obj["action"] | "";
    String target = obj["target"] | "";

    if (action.isEmpty() || target.isEmpty()) {
        error = "Action requires 'action' and 'target' fields";
        return false;
    }

    const Circuit* circuit = findCircuit(target.c_str());
    if (!circuit) {
        error = "Unknown circuit: " + target;
        return false;
    }

    const Device* device = findDevice(circuit->device_id);
    if (!device) {
        error = "Unknown device for circuit: " + target;
        return false;
    }

    const std::string action_name = action.c_str();
    if (action_name != "flash_stop" && !circuitSupports(*circuit, action_name)) {
        error = "Action not supported by circuit";
        return false;
    }

    if (action == "toggle") {
        return applyToggle(*circuit, *device, obj, error, response);
    }
    if (action == "momentary") {
        return applyMomentary(*circuit, *device, obj, error, response);
    }
    if (action == "timed") {
        return applyTimed(*circuit, *device, obj, error, response);
    }
    if (action == "flash") {
        return applyFlash(*circuit, *device, obj, false, error, response);
    }
    if (action == "flash_timed") {
        return applyFlash(*circuit, *device, obj, true, error, response);
    }
    if (action == "flash_stop") {
        return applyFlashStop(*circuit, *device, error, response);
    }
    if (action == "fade") {
        return applyFade(*circuit, *device, obj, error, response);
    }

    error = "Unsupported action";
    return false;
}

bool Ipm1CanSystem::applyToggle(const Circuit& circuit, const Device& device, JsonVariantConst action_json, String& error, JsonObject response) {
    if (!circuit.output_is_numeric) {
        error = "Toggle requires a numeric output";
        return false;
    }
    bool desired_on = false;
    bool has_state = false;
    JsonVariantConst state_value = action_json["state"];
    if (!state_value.isNull()) {
        has_state = parseBool(state_value, desired_on);
    }

    CircuitState* state = findState(circuit.name);
    if (!has_state) {
        bool current = state ? state->is_on : false;
        desired_on = !current;
    }

    if (!sendPowercellValue(device, circuit.output_number, desired_on ? 0xFF : 0x00, error)) {
        return false;
    }

    if (state) {
        state->is_on = desired_on;
        state->pwm = desired_on ? 0xFF : 0x00;
    }

    cancelActiveAction(circuit.name);
    response["circuit"] = circuit.name.c_str();
    response["state"] = desired_on ? "on" : "off";
    response["owner"] = "toggle";
    return true;
}

bool Ipm1CanSystem::applyMomentary(const Circuit& circuit, const Device& device, JsonVariantConst action_json, String& error, JsonObject response) {
    if (!circuit.output_is_numeric) {
        error = "Momentary requires a numeric output";
        return false;
    }

    bool pressed = false;
    bool has_value = false;
    if (action_json.containsKey("pressed")) {
        has_value = parseBool(action_json["pressed"], pressed);
    } else if (action_json.containsKey("state")) {
        has_value = parseBool(action_json["state"], pressed);
    }

    if (!has_value) {
        error = "Momentary requires 'pressed' or 'state'";
        return false;
    }

    if (!sendPowercellValue(device, circuit.output_number, pressed ? 0xFF : 0x00, error)) {
        return false;
    }

    CircuitState* state = findState(circuit.name);
    if (state) {
        state->is_on = pressed;
        state->pwm = pressed ? 0xFF : 0x00;
    }

    cancelActiveAction(circuit.name);
    response["circuit"] = circuit.name.c_str();
    response["state"] = pressed ? "on" : "off";
    response["owner"] = "momentary";
    return true;
}

bool Ipm1CanSystem::applyTimed(const Circuit& circuit, const Device& device, JsonVariantConst action_json, String& error, JsonObject response) {
    if (!circuit.output_is_numeric) {
        error = "Timed action requires a numeric output";
        return false;
    }

    std::uint32_t duration_ms = action_json["duration_ms"] | 0;
    if (duration_ms == 0) {
        error = "Timed action requires duration_ms";
        return false;
    }

    cancelActiveAction(circuit.name);

    if (!sendPowercellValue(device, circuit.output_number, 0xFF, error)) {
        return false;
    }

    CircuitState* state = findState(circuit.name);
    if (state) {
        state->is_on = true;
        state->pwm = 0xFF;
    }

    auto* ctx = new ActionTaskContext{};
    ctx->system = this;
    ctx->circuit = circuit.name;
    ctx->address = device.address;
    ctx->output = circuit.output_number;
    ctx->duration_ms = duration_ms;

    TaskHandle_t task = nullptr;
    if (xTaskCreate(timedTask, "ipm1_timed", 4096, ctx, 1, &task) != pdTRUE) {
        delete ctx;
        error = "Failed to start timed task";
        return false;
    }

    setActiveAction(circuit.name, "timed", task);
    response["circuit"] = circuit.name.c_str();
    response["state"] = "on";
    response["owner"] = "timed";
    response["duration_ms"] = duration_ms;
    return true;
}

bool Ipm1CanSystem::applyFlash(const Circuit& circuit, const Device& device, JsonVariantConst action_json, bool timed, String& error, JsonObject response) {
    if (!circuit.output_is_numeric) {
        error = "Flash action requires a numeric output";
        return false;
    }

    std::uint32_t period_ms = action_json["period_ms"] | 0;
    if (period_ms == 0) {
        error = "Flash action requires period_ms";
        return false;
    }

    std::uint32_t duration_ms = 0;
    if (timed) {
        duration_ms = action_json["duration_ms"] | 0;
        if (duration_ms == 0) {
            error = "Flash timed requires duration_ms";
            return false;
        }
    }

    cancelActiveAction(circuit.name);

    auto* ctx = new ActionTaskContext{};
    ctx->system = this;
    ctx->circuit = circuit.name;
    ctx->address = device.address;
    ctx->output = circuit.output_number;
    ctx->period_ms = period_ms;
    ctx->duration_ms = duration_ms;

    TaskHandle_t task = nullptr;
    if (xTaskCreate(flashTask, "ipm1_flash", 4096, ctx, 1, &task) != pdTRUE) {
        delete ctx;
        error = "Failed to start flash task";
        return false;
    }

    setActiveAction(circuit.name, timed ? "flash_timed" : "flash", task);
    response["circuit"] = circuit.name.c_str();
    response["owner"] = timed ? "flash_timed" : "flash";
    response["period_ms"] = period_ms;
    if (timed) {
        response["duration_ms"] = duration_ms;
    }
    return true;
}

bool Ipm1CanSystem::applyFlashStop(const Circuit& circuit, const Device& device, String& error, JsonObject response) {
    if (!circuit.output_is_numeric) {
        error = "Flash stop requires a numeric output";
        return false;
    }

    cancelActiveAction(circuit.name);
    if (!sendPowercellValue(device, circuit.output_number, 0x00, error)) {
        return false;
    }

    CircuitState* state = findState(circuit.name);
    if (state) {
        state->is_on = false;
        state->pwm = 0;
    }

    response["circuit"] = circuit.name.c_str();
    response["state"] = "off";
    response["owner"] = "none";
    return true;
}

bool Ipm1CanSystem::applyFade(const Circuit& circuit, const Device& device, JsonVariantConst action_json, String& error, JsonObject response) {
    if (!circuit.output_is_numeric) {
        error = "Fade action requires a numeric output";
        return false;
    }

    std::uint32_t duration_ms = action_json["duration_ms"] | 0;
    std::uint8_t target_pwm = action_json["target_pwm"] | 0;
    if (duration_ms == 0) {
        error = "Fade action requires duration_ms";
        return false;
    }

    CircuitState* state = findState(circuit.name);
    std::uint8_t start_pwm = state ? state->pwm : 0;

    cancelActiveAction(circuit.name);

    auto* ctx = new ActionTaskContext{};
    ctx->system = this;
    ctx->circuit = circuit.name;
    ctx->address = device.address;
    ctx->output = circuit.output_number;
    ctx->start_pwm = start_pwm;
    ctx->target_pwm = target_pwm;
    ctx->fade_ms = duration_ms;

    TaskHandle_t task = nullptr;
    if (xTaskCreate(fadeTask, "ipm1_fade", 4096, ctx, 1, &task) != pdTRUE) {
        delete ctx;
        error = "Failed to start fade task";
        return false;
    }

    setActiveAction(circuit.name, "fade", task);
    response["circuit"] = circuit.name.c_str();
    response["owner"] = "fade";
    response["target_pwm"] = target_pwm;
    response["duration_ms"] = duration_ms;
    return true;
}

bool Ipm1CanSystem::sendPowercellValue(const Device& device, std::int32_t output, std::uint8_t value, String& error) {
    if (device.type != "powercell") {
        error = "Device type not supported for output control";
        return false;
    }

    if (output < 1 || output > 8) {
        error = "Powercell output out of range (1-8)";
        return false;
    }

    CanFrameConfig frame = Ipm1Can::powercellOutput(device.address, static_cast<std::uint8_t>(output), value);
    return CanManager::instance().sendFrame(frame);
}

void Ipm1CanSystem::cancelActiveAction(const std::string& circuit) {
    for (auto it = active_actions_.begin(); it != active_actions_.end(); ++it) {
        if (it->circuit == circuit) {
            if (it->task) {
                vTaskDelete(it->task);
            }
            active_actions_.erase(it);
            break;
        }
    }
}

void Ipm1CanSystem::setActiveAction(const std::string& circuit, const std::string& action, TaskHandle_t task) {
    ActiveAction active;
    active.circuit = circuit;
    active.action = action;
    active.task = task;
    active_actions_.push_back(active);
}

void Ipm1CanSystem::clearActiveAction(const std::string& circuit) {
    for (auto it = active_actions_.begin(); it != active_actions_.end(); ++it) {
        if (it->circuit == circuit) {
            active_actions_.erase(it);
            break;
        }
    }
}

void Ipm1CanSystem::timedTask(void* pv) {
    auto* ctx = static_cast<ActionTaskContext*>(pv);
    if (!ctx || !ctx->system) {
        vTaskDelete(nullptr);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(ctx->duration_ms));

    String error;
    Ipm1CanSystem::Device device;
    device.type = "powercell";
    device.address = ctx->address;
    ctx->system->sendPowercellValue(device, ctx->output, ctx->off_value, error);

    if (auto* state = ctx->system->findState(ctx->circuit)) {
        state->is_on = false;
        state->pwm = 0;
    }

    ctx->system->clearActiveAction(ctx->circuit);
    delete ctx;
    vTaskDelete(nullptr);
}

void Ipm1CanSystem::flashTask(void* pv) {
    auto* ctx = static_cast<ActionTaskContext*>(pv);
    if (!ctx || !ctx->system || ctx->period_ms == 0) {
        vTaskDelete(nullptr);
        return;
    }

    const std::uint32_t start_ms = millis();
    bool on = false;
    while (true) {
        if (ctx->duration_ms > 0 && (millis() - start_ms) >= ctx->duration_ms) {
            break;
        }

        on = !on;
        String error;
        Ipm1CanSystem::Device device;
        device.type = "powercell";
        device.address = ctx->address;
        ctx->system->sendPowercellValue(device, ctx->output, on ? ctx->on_value : ctx->off_value, error);

        if (auto* state = ctx->system->findState(ctx->circuit)) {
            state->is_on = on;
            state->pwm = on ? ctx->on_value : ctx->off_value;
        }

        vTaskDelay(pdMS_TO_TICKS(ctx->period_ms));
    }

    String error;
    Ipm1CanSystem::Device device;
    device.type = "powercell";
    device.address = ctx->address;
    ctx->system->sendPowercellValue(device, ctx->output, ctx->off_value, error);

    if (auto* state = ctx->system->findState(ctx->circuit)) {
        state->is_on = false;
        state->pwm = 0;
    }

    ctx->system->clearActiveAction(ctx->circuit);
    delete ctx;
    vTaskDelete(nullptr);
}

void Ipm1CanSystem::fadeTask(void* pv) {
    auto* ctx = static_cast<ActionTaskContext*>(pv);
    if (!ctx || !ctx->system || ctx->fade_ms == 0) {
        vTaskDelete(nullptr);
        return;
    }

    const int steps = 20;
    const std::uint32_t step_delay = ctx->fade_ms / steps;
    for (int i = 1; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        std::uint8_t value = static_cast<std::uint8_t>(ctx->start_pwm + (ctx->target_pwm - ctx->start_pwm) * t);
        String error;
        Ipm1CanSystem::Device device;
        device.type = "powercell";
        device.address = ctx->address;
        ctx->system->sendPowercellValue(device, ctx->output, value, error);
        if (auto* state = ctx->system->findState(ctx->circuit)) {
            state->is_on = value > 0;
            state->pwm = value;
        }
        vTaskDelay(pdMS_TO_TICKS(step_delay > 0 ? step_delay : 1));
    }

    ctx->system->clearActiveAction(ctx->circuit);
    delete ctx;
    vTaskDelete(nullptr);
}
