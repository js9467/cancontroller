/**
 * @file infinitybox_control.h
 * @brief Infinitybox IPM1 function registry and control system
 * 
 * This module implements the complete UI behavioral model for Infinitybox IPM1:
 * - Function database from JSON schema
 * - Behavior assignment (toggle, momentary, flash, fade, timed, scene, one_shot)
 * - Ownership and conflict management
 * - Feedback integration (current draw, fault state)
 * 
 * Design principles:
 * 1. ADDITIVE: Does not replace existing functionality
 * 2. Functions are fixed by IPM1 assignments
 * 3. Behaviors are assigned by UI
 * 4. One owner per function at a time
 * 5. Actual state comes from CAN feedback, not assumptions
 */

#pragma once

#include <Arduino.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

// Forward declaration
class Ipm1CanSystem;

namespace InfinityboxControl {

// ===== ENUMS =====

enum class DeviceType : uint8_t {
    POWERCELL,
    INMOTION,
    MASTERCELL
};

enum class BehaviorType : uint8_t {
    TOGGLE,          // ON/OFF switch - user controls
    MOMENTARY,       // Press & hold - active while pressed
    FLASH,           // Continuous flashing - managed by flash engine
    FLASH_TIMED,     // Flash for duration then stop
    FADE,            // PWM ramp up/down
    TIMED,           // One-shot timed pulse
    SCENE,           // Scene-controlled state
    ONE_SHOT         // Single pulse (for door locks)
};

enum class OwnerType : uint8_t {
    NONE,            // No active owner
    MANUAL,          // User direct control (toggle/momentary)
    FLASH_ENGINE,    // Flash behavior active
    TIMER,           // Timed behavior active
    SCENE,           // Scene is controlling
    FADE_ENGINE      // Fade behavior active
};

enum class FunctionState : uint8_t {
    OFF,
    ON,
    FLASHING,
    FADING,
    FAULT
};

// ===== STRUCTURES =====

struct Device {
    std::string id;           // e.g., "pc_front"
    DeviceType type;
    uint8_t address;          // CAN address (0 for mastercell)
    std::string name;         // Human-readable
};

struct OutputReference {
    std::string device_id;    // Which device
    uint8_t output_num;       // Output number (1-10 for powercell, special for inmotion)
    std::string output_name;  // e.g., "relay_1a", "aux_03"
};

struct Function {
    std::string name;                         // e.g., "Headlights"
    std::vector<OutputReference> outputs;     // Single or multiple outputs (4-ways)
    std::vector<BehaviorType> allowed_behaviors;
    std::vector<std::string> requires;        // e.g., ["ignition"]
    std::vector<std::string> blocked_when;    // e.g., ["security"]
    bool renameable;
    
    // Runtime state
    BehaviorType active_behavior;
    OwnerType current_owner;
    FunctionState state;
    float current_draw_amps;                  // From CAN feedback
    bool fault_detected;
    uint32_t owner_start_ms;                  // When current owner took control
    
    Function() : renameable(false), 
                 active_behavior(BehaviorType::TOGGLE),
                 current_owner(OwnerType::NONE),
                 state(FunctionState::OFF),
                 current_draw_amps(0.0f),
                 fault_detected(false),
                 owner_start_ms(0) {}
};

// ===== BEHAVIOR CONFIGURATION =====

struct FlashConfig {
    uint16_t on_time_ms;      // Default 500ms
    uint16_t off_time_ms;     // Default 500ms
    uint32_t duration_ms;     // 0 = continuous
    
    FlashConfig() : on_time_ms(500), off_time_ms(500), duration_ms(0) {}
};

struct FadeConfig {
    uint8_t target_level;     // 0-100%
    uint16_t duration_ms;     // Ramp time
    
    FadeConfig() : target_level(100), duration_ms(1000) {}
};

struct TimedConfig {
    uint16_t duration_ms;     // Pulse duration
    
    TimedConfig() : duration_ms(500) {}
};

// ===== SCENE =====

struct SceneAction {
    std::string function_name;
    BehaviorType behavior;
    bool target_state;        // ON/OFF for toggle
    uint8_t level;            // 0-100 for fade
};

struct Scene {
    std::string name;
    std::vector<SceneAction> actions;
    bool active;
    
    Scene() : active(false) {}
};

// ===== MAIN CONTROL CLASS =====

class InfinityboxController {
public:
    static InfinityboxController& instance() {
        static InfinityboxController inst;
        return inst;
    }
    
    // Initialization
    bool begin(Ipm1CanSystem* can_system);
    void loop();  // Call from main loop for behavior engines
    
    // Device management
    bool addDevice(const Device& device);
    const Device* getDevice(const std::string& device_id) const;
    
    // Function management
    bool addFunction(const Function& func);
    Function* getFunction(const std::string& name);
    const Function* getFunction(const std::string& name) const;
    std::vector<std::string> getAllFunctionNames() const;
    std::vector<std::string> getFunctionsByCategory(const std::string& category) const;
    
    // Function control (returns false if blocked or conflict)
    bool activateFunction(const std::string& name, bool state);
    bool activateFunctionWithBehavior(const std::string& name, BehaviorType behavior, bool state = true);
    bool activateFunctionFade(const std::string& name, uint8_t level, uint16_t duration_ms = 1000);
    bool activateFunctionFlash(const std::string& name, uint16_t on_ms, uint16_t off_ms, uint32_t duration_ms = 0);
    bool deactivateFunction(const std::string& name);
    
    // Ownership queries
    bool canActivate(const std::string& name, OwnerType requesting_owner) const;
    void releaseOwnership(const std::string& name);
    
    // Security
    void setSecurityActive(bool active);
    bool isSecurityActive() const { return m_security_active; }
    
    // Ignition (affects functions with "requires": ["ignition"])
    void setIgnitionOn(bool on);
    bool isIgnitionOn() const { return m_ignition_on; }
    
    // Scene management
    bool addScene(const Scene& scene);
    bool activateScene(const std::string& scene_name);
    bool deactivateScene(const std::string& scene_name);
    Scene* getScene(const std::string& scene_name);
    
    // Feedback integration
    void updateFunctionFeedback(const std::string& name, float current_amps, bool fault);
    
    // Behavior configuration
    void setFlashConfig(const FlashConfig& config) { m_flash_config = config; }
    FlashConfig getFlashConfig() const { return m_flash_config; }
    
    // Debug/testing
    void printStatus() const;
    
private:
    InfinityboxController();
    ~InfinityboxController() = default;
    InfinityboxController(const InfinityboxController&) = delete;
    InfinityboxController& operator=(const InfinityboxController&) = delete;
    
    // Internal helpers
    bool isBlocked(const Function& func) const;
    bool sendCanCommand(const Function& func, bool state);
    bool sendCanCommandMultiOutput(const Function& func, bool state);
    void updateFlashEngines();
    void updateFadeEngines();
    void updateTimedEngines();
    
    // Data
    std::map<std::string, Device> m_devices;
    std::map<std::string, Function> m_functions;
    std::map<std::string, Scene> m_scenes;
    
    Ipm1CanSystem* m_can_system;
    bool m_security_active;
    bool m_ignition_on;
    
    FlashConfig m_flash_config;
    
    // Behavior engine tracking
    struct FlashState {
        uint32_t last_toggle_ms;
        bool current_state;
    };
    std::map<std::string, FlashState> m_flash_states;
    
    struct FadeState {
        uint32_t start_ms;
        uint8_t start_level;
        uint8_t target_level;
        uint16_t duration_ms;
    };
    std::map<std::string, FadeState> m_fade_states;
    
    struct TimedState {
        uint32_t start_ms;
        uint16_t duration_ms;
    };
    std::map<std::string, TimedState> m_timed_states;
};

// ===== CATEGORY DEFINITIONS =====
// These map to the UI navigation structure

namespace Categories {
    constexpr const char* DRIVING = "Driving";
    constexpr const char* EXTERIOR = "Exterior Lighting";
    constexpr const char* INTERIOR = "Interior";
    constexpr const char* BODY = "Body";
    constexpr const char* POWERTRAIN = "Powertrain";
    constexpr const char* AUX = "AUX / Custom";
    constexpr const char* INDICATORS = "Indicators";
}

// Helper: Convert string to BehaviorType
BehaviorType stringToBehavior(const std::string& str);
const char* behaviorToString(BehaviorType behavior);
const char* ownerTypeToString(OwnerType owner);
const char* functionStateToString(FunctionState state);

} // namespace InfinityboxControl
