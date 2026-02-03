/**
 * @file infinitybox_control.cpp
 * @brief Implementation of Infinitybox IPM1 control system
 */

#include "infinitybox_control.h"
#include "ipm1_can_system.h"
#include <ArduinoJson.h>
#include <algorithm>

namespace InfinityboxControl {

// ===== UTILITY FUNCTIONS =====

BehaviorType stringToBehavior(const std::string& str) {
    if (str == "toggle") return BehaviorType::TOGGLE;
    if (str == "momentary") return BehaviorType::MOMENTARY;
    if (str == "flash") return BehaviorType::FLASH;
    if (str == "flash_timed") return BehaviorType::FLASH_TIMED;
    if (str == "fade") return BehaviorType::FADE;
    if (str == "timed") return BehaviorType::TIMED;
    if (str == "scene") return BehaviorType::SCENE;
    if (str == "one_shot") return BehaviorType::ONE_SHOT;
    return BehaviorType::TOGGLE;  // Default
}

const char* behaviorToString(BehaviorType behavior) {
    switch (behavior) {
        case BehaviorType::TOGGLE: return "toggle";
        case BehaviorType::MOMENTARY: return "momentary";
        case BehaviorType::FLASH: return "flash";
        case BehaviorType::FLASH_TIMED: return "flash_timed";
        case BehaviorType::FADE: return "fade";
        case BehaviorType::TIMED: return "timed";
        case BehaviorType::SCENE: return "scene";
        case BehaviorType::ONE_SHOT: return "one_shot";
        default: return "unknown";
    }
}

const char* ownerTypeToString(OwnerType owner) {
    switch (owner) {
        case OwnerType::NONE: return "none";
        case OwnerType::MANUAL: return "manual";
        case OwnerType::FLASH_ENGINE: return "flash_engine";
        case OwnerType::TIMER: return "timer";
        case OwnerType::SCENE: return "scene";
        case OwnerType::FADE_ENGINE: return "fade_engine";
        default: return "unknown";
    }
}

const char* functionStateToString(FunctionState state) {
    switch (state) {
        case FunctionState::OFF: return "OFF";
        case FunctionState::ON: return "ON";
        case FunctionState::FLASHING: return "FLASHING";
        case FunctionState::FADING: return "FADING";
        case FunctionState::FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

// ===== MAIN IMPLEMENTATION =====

InfinityboxController::InfinityboxController() 
    : m_can_system(nullptr)
    , m_security_active(false)
    , m_ignition_on(false)
{
}

bool InfinityboxController::begin(Ipm1CanSystem* can_system) {
    if (!can_system) {
        Serial.println("[IBOX] ERROR: CAN system is null");
        return false;
    }
    
    m_can_system = can_system;
    
    // Load default JSON schema
    // Device definitions
    Device pc_front = {"pc_front", DeviceType::POWERCELL, 1, "Front Powercell"};
    Device pc_rear = {"pc_rear", DeviceType::POWERCELL, 2, "Rear Powercell"};
    Device im_df = {"im_df", DeviceType::INMOTION, 3, "Driver Front inMotion"};
    Device im_pf = {"im_pf", DeviceType::INMOTION, 4, "Passenger Front inMotion"};
    Device im_dr = {"im_dr", DeviceType::INMOTION, 5, "Driver Rear inMotion"};
    Device im_pr = {"im_pr", DeviceType::INMOTION, 6, "Passenger Rear inMotion"};
    Device mastercell = {"mastercell", DeviceType::MASTERCELL, 0, "Mastercell"};
    
    addDevice(pc_front);
    addDevice(pc_rear);
    addDevice(im_df);
    addDevice(im_pf);
    addDevice(im_dr);
    addDevice(im_pr);
    addDevice(mastercell);
    
    // Function definitions from JSON
    // Turn signals
    {
        Function f;
        f.name = "Left Turn Signal Front";
        f.outputs = {{"pc_front", 1, "output_1"}};
        f.allowed_behaviors = {BehaviorType::FLASH, BehaviorType::FLASH_TIMED};
        f.requires = {"ignition"};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Right Turn Signal Front";
        f.outputs = {{"pc_front", 2, "output_2"}};
        f.allowed_behaviors = {BehaviorType::FLASH, BehaviorType::FLASH_TIMED};
        f.requires = {"ignition"};
        addFunction(f);
    }
    {
        Function f;
        f.name = "4-Ways";
        f.outputs = {{"pc_front", 1, "output_1"}, {"pc_front", 2, "output_2"}};
        f.allowed_behaviors = {BehaviorType::FLASH};
        addFunction(f);
    }
    
    // Powertrain
    {
        Function f;
        f.name = "Ignition";
        f.outputs = {{"pc_front", 3, "output_3"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Starter";
        f.outputs = {{"pc_front", 4, "output_4"}};
        f.allowed_behaviors = {BehaviorType::MOMENTARY};
        f.blocked_when = {"security"};
        addFunction(f);
    }
    
    // Lighting
    {
        Function f;
        f.name = "Headlights";
        f.outputs = {{"pc_front", 5, "output_5"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE, BehaviorType::SCENE, BehaviorType::FADE};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Parking Lights Front";
        f.outputs = {{"pc_front", 6, "output_6"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE};
        addFunction(f);
    }
    {
        Function f;
        f.name = "High Beams";
        f.outputs = {{"pc_front", 7, "output_7"}};
        f.allowed_behaviors = {BehaviorType::MOMENTARY, BehaviorType::TOGGLE};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Horn";
        f.outputs = {{"pc_front", 9, "output_9"}};
        f.allowed_behaviors = {BehaviorType::MOMENTARY};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Cooling Fan";
        f.outputs = {{"pc_front", 10, "output_10"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE, BehaviorType::TIMED};
        addFunction(f);
    }
    
    // Rear lighting
    {
        Function f;
        f.name = "Left Turn Signal Rear";
        f.outputs = {{"pc_rear", 1, "output_1"}};
        f.allowed_behaviors = {BehaviorType::FLASH, BehaviorType::FLASH_TIMED};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Right Turn Signal Rear";
        f.outputs = {{"pc_rear", 2, "output_2"}};
        f.allowed_behaviors = {BehaviorType::FLASH, BehaviorType::FLASH_TIMED};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Brake Lights";
        f.outputs = {{"pc_rear", 3, "output_3"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Interior Lights";
        f.outputs = {{"pc_rear", 4, "output_4"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE, BehaviorType::FADE, BehaviorType::TIMED};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Backup Lights";
        f.outputs = {{"pc_rear", 5, "output_5"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Parking Lights Rear";
        f.outputs = {{"pc_rear", 6, "output_6"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Fuel Pump";
        f.outputs = {{"pc_rear", 10, "output_10"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE};
        f.blocked_when = {"security"};
        addFunction(f);
    }
    
    // Window controls
    {
        Function f;
        f.name = "Driver Window Up";
        f.outputs = {{"im_df", 0, "relay_1a"}};
        f.allowed_behaviors = {BehaviorType::MOMENTARY};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Driver Window Down";
        f.outputs = {{"im_df", 0, "relay_1b"}};
        f.allowed_behaviors = {BehaviorType::MOMENTARY};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Passenger Window Up";
        f.outputs = {{"im_pf", 0, "relay_1a"}};
        f.allowed_behaviors = {BehaviorType::MOMENTARY};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Passenger Window Down";
        f.outputs = {{"im_pf", 0, "relay_1b"}};
        f.allowed_behaviors = {BehaviorType::MOMENTARY};
        addFunction(f);
    }
    
    // Door locks
    {
        Function f;
        f.name = "Driver Door Lock";
        f.outputs = {{"im_df", 0, "relay_2a"}};
        f.allowed_behaviors = {BehaviorType::ONE_SHOT};
        addFunction(f);
    }
    {
        Function f;
        f.name = "Driver Door Unlock";
        f.outputs = {{"im_df", 0, "relay_2b"}};
        f.allowed_behaviors = {BehaviorType::ONE_SHOT};
        addFunction(f);
    }
    
    // AUX outputs (renameable)
    {
        Function f;
        f.name = "AUX 03";
        f.outputs = {{"im_df", 0, "aux_03"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE, BehaviorType::FLASH, BehaviorType::FADE, BehaviorType::TIMED};
        f.renameable = true;
        addFunction(f);
    }
    {
        Function f;
        f.name = "AUX 04";
        f.outputs = {{"im_df", 0, "aux_04"}};
        f.allowed_behaviors = {BehaviorType::TOGGLE, BehaviorType::FLASH, BehaviorType::FADE, BehaviorType::TIMED};
        f.renameable = true;
        addFunction(f);
    }
    
    Serial.printf("[IBOX] Initialized with %d devices and %d functions\n", 
                  m_devices.size(), m_functions.size());
    return true;
}

void InfinityboxController::loop() {
    updateFlashEngines();
    updateFadeEngines();
    updateTimedEngines();
}

bool InfinityboxController::addDevice(const Device& device) {
    m_devices[device.id] = device;
    return true;
}

const Device* InfinityboxController::getDevice(const std::string& device_id) const {
    auto it = m_devices.find(device_id);
    return (it != m_devices.end()) ? &it->second : nullptr;
}

bool InfinityboxController::addFunction(const Function& func) {
    m_functions[func.name] = func;
    return true;
}

Function* InfinityboxController::getFunction(const std::string& name) {
    auto it = m_functions.find(name);
    return (it != m_functions.end()) ? &it->second : nullptr;
}

const Function* InfinityboxController::getFunction(const std::string& name) const {
    auto it = m_functions.find(name);
    return (it != m_functions.end()) ? &it->second : nullptr;
}

std::vector<std::string> InfinityboxController::getAllFunctionNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_functions) {
        names.push_back(pair.first);
    }
    return names;
}

bool InfinityboxController::isBlocked(const Function& func) const {
    // Check security blocks
    if (m_security_active) {
        for (const auto& block : func.blocked_when) {
            if (block == "security") {
                return true;
            }
        }
    }
    
    // Check ignition requirements
    if (!m_ignition_on) {
        for (const auto& req : func.requires) {
            if (req == "ignition") {
                return true;
            }
        }
    }
    
    return false;
}

bool InfinityboxController::canActivate(const std::string& name, OwnerType requesting_owner) const {
    const Function* func = getFunction(name);
    if (!func) return false;
    
    // Check if blocked by security/ignition
    if (isBlocked(*func)) return false;
    
    // Check current owner
    if (func->current_owner == OwnerType::NONE) return true;
    if (func->current_owner == requesting_owner) return true;
    
    // Another owner has control
    return false;
}

void InfinityboxController::releaseOwnership(const std::string& name) {
    Function* func = getFunction(name);
    if (!func) return;
    
    func->current_owner = OwnerType::NONE;
    func->owner_start_ms = 0;
    
    // Clean up behavior engine state
    m_flash_states.erase(name);
    m_fade_states.erase(name);
    m_timed_states.erase(name);
}

bool InfinityboxController::activateFunction(const std::string& name, bool state) {
    Function* func = getFunction(name);
    if (!func) {
        Serial.printf("[IBOX] Function '%s' not found\n", name.c_str());
        return false;
    }
    
    // Use first allowed behavior as default
    if (func->allowed_behaviors.empty()) {
        Serial.printf("[IBOX] Function '%s' has no allowed behaviors\n", name.c_str());
        return false;
    }
    
    return activateFunctionWithBehavior(name, func->allowed_behaviors[0], state);
}

bool InfinityboxController::activateFunctionWithBehavior(const std::string& name, BehaviorType behavior, bool state) {
    Function* func = getFunction(name);
    if (!func) return false;
    
    // Check if behavior is allowed
    if (std::find(func->allowed_behaviors.begin(), func->allowed_behaviors.end(), behavior) 
        == func->allowed_behaviors.end()) {
        Serial.printf("[IBOX] Behavior %s not allowed for %s\n", behaviorToString(behavior), name.c_str());
        return false;
    }
    
    // Determine owner type
    OwnerType owner = OwnerType::MANUAL;
    if (behavior == BehaviorType::FLASH || behavior == BehaviorType::FLASH_TIMED) {
        owner = OwnerType::FLASH_ENGINE;
    } else if (behavior == BehaviorType::FADE) {
        owner = OwnerType::FADE_ENGINE;
    } else if (behavior == BehaviorType::TIMED || behavior == BehaviorType::ONE_SHOT) {
        owner = OwnerType::TIMER;
    }
    
    // Check if we can activate
    if (!canActivate(name, owner)) {
        Serial.printf("[IBOX] Cannot activate %s - blocked or owned\n", name.c_str());
        return false;
    }
    
    // Release previous owner if different
    if (func->current_owner != owner && func->current_owner != OwnerType::NONE) {
        releaseOwnership(name);
    }
    
    // Set new owner
    func->current_owner = owner;
    func->active_behavior = behavior;
    func->owner_start_ms = millis();
    
    // Execute behavior
    switch (behavior) {
        case BehaviorType::TOGGLE:
        case BehaviorType::MOMENTARY:
            func->state = state ? FunctionState::ON : FunctionState::OFF;
            return sendCanCommand(*func, state);
            
        case BehaviorType::FLASH:
        case BehaviorType::FLASH_TIMED:
            if (state) {
                func->state = FunctionState::FLASHING;
                FlashState fs;
                fs.last_toggle_ms = millis();
                fs.current_state = false;
                m_flash_states[name] = fs;
                return true;  // Flash engine will handle
            } else {
                func->state = FunctionState::OFF;
                m_flash_states.erase(name);
                return sendCanCommand(*func, false);
            }
            
        case BehaviorType::TIMED:
        case BehaviorType::ONE_SHOT:
            func->state = FunctionState::ON;
            TimedState ts;
            ts.start_ms = millis();
            ts.duration_ms = 500;  // Default 500ms
            m_timed_states[name] = ts;
            return sendCanCommand(*func, true);
            
        default:
            return false;
    }
}

bool InfinityboxController::activateFunctionFade(const std::string& name, uint8_t level, uint16_t duration_ms) {
    Function* func = getFunction(name);
    if (!func) return false;
    
    if (std::find(func->allowed_behaviors.begin(), func->allowed_behaviors.end(), BehaviorType::FADE) 
        == func->allowed_behaviors.end()) {
        return false;
    }
    
    if (!canActivate(name, OwnerType::FADE_ENGINE)) return false;
    
    func->current_owner = OwnerType::FADE_ENGINE;
    func->active_behavior = BehaviorType::FADE;
    func->owner_start_ms = millis();
    func->state = FunctionState::FADING;
    
    FadeState fs;
    fs.start_ms = millis();
    fs.start_level = 0;  // TODO: Get current level from feedback
    fs.target_level = level;
    fs.duration_ms = duration_ms;
    m_fade_states[name] = fs;
    
    return true;
}

bool InfinityboxController::activateFunctionFlash(const std::string& name, uint16_t on_ms, uint16_t off_ms, uint32_t duration_ms) {
    Function* func = getFunction(name);
    if (!func) return false;
    
    BehaviorType behavior = (duration_ms > 0) ? BehaviorType::FLASH_TIMED : BehaviorType::FLASH;
    
    if (std::find(func->allowed_behaviors.begin(), func->allowed_behaviors.end(), behavior) 
        == func->allowed_behaviors.end()) {
        return false;
    }
    
    if (!canActivate(name, OwnerType::FLASH_ENGINE)) return false;
    
    func->current_owner = OwnerType::FLASH_ENGINE;
    func->active_behavior = behavior;
    func->owner_start_ms = millis();
    func->state = FunctionState::FLASHING;
    
    FlashState fs;
    fs.last_toggle_ms = millis();
    fs.current_state = false;
    m_flash_states[name] = fs;
    
    // Store custom flash timing (TODO: make per-function)
    m_flash_config.on_time_ms = on_ms;
    m_flash_config.off_time_ms = off_ms;
    m_flash_config.duration_ms = duration_ms;
    
    return true;
}

bool InfinityboxController::deactivateFunction(const std::string& name) {
    Function* func = getFunction(name);
    if (!func) return false;
    
    func->state = FunctionState::OFF;
    sendCanCommand(*func, false);
    releaseOwnership(name);
    
    return true;
}

void InfinityboxController::setSecurityActive(bool active) {
    m_security_active = active;
    Serial.printf("[IBOX] Security %s\n", active ? "ACTIVE" : "INACTIVE");
    
    // Deactivate blocked functions if security just activated
    if (active) {
        for (auto& pair : m_functions) {
            Function& func = pair.second;
            for (const auto& block : func.blocked_when) {
                if (block == "security" && func.state != FunctionState::OFF) {
                    Serial.printf("[IBOX] Deactivating %s due to security\n", func.name.c_str());
                    deactivateFunction(func.name);
                }
            }
        }
    }
}

void InfinityboxController::setIgnitionOn(bool on) {
    m_ignition_on = on;
    Serial.printf("[IBOX] Ignition %s\n", on ? "ON" : "OFF");
    
    // Deactivate functions requiring ignition if it just turned off
    if (!on) {
        for (auto& pair : m_functions) {
            Function& func = pair.second;
            for (const auto& req : func.requires) {
                if (req == "ignition" && func.state != FunctionState::OFF) {
                    Serial.printf("[IBOX] Deactivating %s due to ignition off\n", func.name.c_str());
                    deactivateFunction(func.name);
                }
            }
        }
    }
}

void InfinityboxController::updateFunctionFeedback(const std::string& name, float current_amps, bool fault) {
    Function* func = getFunction(name);
    if (!func) return;
    
    func->current_draw_amps = current_amps;
    func->fault_detected = fault;
    
    // Update fault state
    if (fault) {
        func->state = FunctionState::FAULT;
    }
}

bool InfinityboxController::sendCanCommand(const Function& func, bool state) {
    if (!m_can_system) return false;
    
    // Send to all outputs for this function
    for (const auto& output : func.outputs) {
        const Device* device = getDevice(output.device_id);
        if (!device) {
            Serial.printf("[IBOX] Device %s not found\n", output.device_id.c_str());
            continue;
        }
        
        // Prepare JSON action for IPM1 system
        StaticJsonDocument<256> doc;
        JsonObject action = doc.to<JsonObject>();
        
        action["circuit"] = func.name;
        action["action"] = "toggle";
        
        JsonObject params = action.createNestedObject("params");
        params["state"] = state ? "on" : "off";
        
        String error;
        StaticJsonDocument<128> responseDoc;
        JsonObject response = responseDoc.to<JsonObject>();
        
        // Pass action as JsonVariantConst
        if (!m_can_system->handleAction(action, error, response)) {
            Serial.printf("[IBOX] CAN send failed: %s\n", error.c_str());
            continue;
        }
        
        Serial.printf("[IBOX] CAN: %s (dev=%s addr=%d out=%d) -> %s\n",
                      func.name.c_str(), device->id.c_str(), device->address,
                      output.output_num, state ? "ON" : "OFF");
    }
    
    return true;
}

void InfinityboxController::updateFlashEngines() {
    uint32_t now_ms = millis();
    
    for (auto& pair : m_flash_states) {
        const std::string& name = pair.first;
        FlashState& fs = pair.second;
        Function* func = getFunction(name);
        if (!func) continue;
        
        // Check if flash timed out
        if (func->active_behavior == BehaviorType::FLASH_TIMED && 
            m_flash_config.duration_ms > 0) {
            if (now_ms - func->owner_start_ms >= m_flash_config.duration_ms) {
                deactivateFunction(name);
                continue;
            }
        }
        
        // Toggle logic
        uint16_t interval = fs.current_state ? m_flash_config.on_time_ms : m_flash_config.off_time_ms;
        if (now_ms - fs.last_toggle_ms >= interval) {
            fs.current_state = !fs.current_state;
            fs.last_toggle_ms = now_ms;
            sendCanCommand(*func, fs.current_state);
        }
    }
}

void InfinityboxController::updateFadeEngines() {
    uint32_t now_ms = millis();
    
    for (auto it = m_fade_states.begin(); it != m_fade_states.end(); ) {
        const std::string& name = it->first;
        FadeState& fs = it->second;
        Function* func = getFunction(name);
        
        if (!func) {
            it = m_fade_states.erase(it);
            continue;
        }
        
        uint32_t elapsed = now_ms - fs.start_ms;
        if (elapsed >= fs.duration_ms) {
            // Fade complete
            func->state = (fs.target_level > 0) ? FunctionState::ON : FunctionState::OFF;
            sendCanCommand(*func, fs.target_level > 0);
            it = m_fade_states.erase(it);
        } else {
            // Calculate current level
            uint8_t current_level = fs.start_level + 
                ((fs.target_level - fs.start_level) * elapsed) / fs.duration_ms;
            
            // TODO: Send PWM level via CAN
            Serial.printf("[IBOX] Fade %s: %d%%\n", name.c_str(), current_level);
            ++it;
        }
    }
}

void InfinityboxController::updateTimedEngines() {
    uint32_t now_ms = millis();
    
    for (auto it = m_timed_states.begin(); it != m_timed_states.end(); ) {
        const std::string& name = it->first;
        TimedState& ts = it->second;
        Function* func = getFunction(name);
        
        if (!func) {
            it = m_timed_states.erase(it);
            continue;
        }
        
        if (now_ms - ts.start_ms >= ts.duration_ms) {
            // Timer expired - turn off
            func->state = FunctionState::OFF;
            sendCanCommand(*func, false);
            releaseOwnership(name);
            it = m_timed_states.erase(it);
        } else {
            ++it;
        }
    }
}

void InfinityboxController::printStatus() const {
    Serial.println("\n=== INFINITYBOX STATUS ===");
    Serial.printf("Security: %s | Ignition: %s\n", 
                  m_security_active ? "ACTIVE" : "INACTIVE",
                  m_ignition_on ? "ON" : "OFF");
    Serial.printf("Devices: %d | Functions: %d\n", m_devices.size(), m_functions.size());
    Serial.printf("Active flash engines: %d\n", m_flash_states.size());
    Serial.printf("Active fade engines: %d\n", m_fade_states.size());
    Serial.printf("Active timers: %d\n", m_timed_states.size());
    
    Serial.println("\nActive Functions:");
    for (const auto& pair : m_functions) {
        const Function& func = pair.second;
        if (func.state != FunctionState::OFF) {
            Serial.printf("  %s: %s (owner=%s, behavior=%s, current=%.2fA%s)\n",
                          func.name.c_str(),
                          functionStateToString(func.state),
                          ownerTypeToString(func.current_owner),
                          behaviorToString(func.active_behavior),
                          func.current_draw_amps,
                          func.fault_detected ? " FAULT" : "");
        }
    }
    Serial.println("========================\n");
}

} // namespace InfinityboxControl
