#pragma once

#include "output_behavior_engine.h"
#include <map>

/**
 * =================================================================
 *  OUTPUT RULE ENGINE
 *
 *  IF/THEN conditional logic layer for behavioral outputs.
 *
 *  A rule watches one output (the trigger) and, when that output
 *  changes state in the configured direction, performs an action on
 *  a second output (or scene).  The effect can be held until:
 *    - a separate "release" output fires, OR
 *    - an auto-release timer expires, OR
 *    - both (whichever comes first)
 *
 *  All logic is output-driven; rules fire whether the trigger was
 *  caused by a button press, a CAN message, or another rule.
 *
 *  Example: door-lock pulse fires → keep alarm LED on → unlock
 *           pulse fires → alarm LED off.
 * =================================================================
 */

// OutputRule lives outside the BehavioralOutput namespace so it is
// easy to forward-declare or reference from persistence code without
// pulling in the whole BehavioralOutput namespace.

struct OutputRule {
    String id;          // Unique identifier  e.g. "rule_lock_led"
    String name;        // Human-readable     e.g. "Lock  Alarm LED"
    bool   enabled = true;

    // ---- TRIGGER -----------------------------------------------
    String trigger_output_id;      // Output to watch
    bool   trigger_on_rise = true; // Fire when output goes active
    bool   trigger_on_fall = false;// Fire when output goes inactive

    // ---- ACTION ------------------------------------------------
    // "on", "off", "toggle", "flash",
    // "scene_activate", "scene_deactivate"
    String action;
    String action_output_id;  // Target output (on / off / toggle / flash)
    String action_scene_id;   // Target scene  (scene_activate / _deactivate)

    // ---- RELEASE -----------------------------------------------
    // All release conditions are optional; any that is set will
    // revert the action when it fires.
    String   release_output_id;     // Output whose edge triggers release (empty = none)
    bool     release_on_rise = true;// Release fires on active edge of that output
    uint32_t release_auto_ms = 0;   // Auto-release timer in ms (0 = disabled)
};

// ---------------------------------------------------------------------------

class OutputRuleEngine {
public:
    OutputRuleEngine() = default;

    // ------------------------------------------------------------------
    // RULE MANAGEMENT
    // ------------------------------------------------------------------

    void addRule(const OutputRule& rule) {
        _rules[rule.id] = rule;
    }

    void removeRule(const String& id) {
        _rules.erase(id);
        _actionTime.erase(id);
    }

    bool updateRule(const OutputRule& rule) {
        if (_rules.find(rule.id) == _rules.end()) return false;
        _rules[rule.id] = rule;
        return true;
    }

    OutputRule* getRule(const String& id) {
        auto it = _rules.find(id);
        return (it != _rules.end()) ? &it->second : nullptr;
    }

    const std::map<String, OutputRule>& getRules() const { return _rules; }

    void clearAll() {
        _rules.clear();
        _prevStates.clear();
        _actionTime.clear();
        _initialized = false;
    }

    // ------------------------------------------------------------------
    // MAIN UPDATE  – call once per behavioral engine update cycle
    // ------------------------------------------------------------------

    void update(BehavioralOutput::BehaviorEngine& engine) {
        if (_rules.empty()) return;

        const auto& outputs = engine.getOutputs();

        // First call: seed previous states without triggering rules
        if (!_initialized) {
            for (const auto& [id, output] : outputs) {
                _prevStates[id] = output.currentState;
            }
            _initialized = true;
            return;
        }

        uint32_t now = millis();

        // --- Edge detection ---
        for (const auto& [outId, output] : outputs) {
            bool prev = false;
            auto pit = _prevStates.find(outId);
            if (pit != _prevStates.end()) prev = pit->second;

            bool curr = output.currentState;
            _prevStates[outId] = curr;

            if (prev == curr) continue; // No edge on this output

            bool riseEdge = curr && !prev;
            bool fallEdge = !curr &&  prev;

            for (auto& [ruleId, rule] : _rules) {
                if (!rule.enabled) continue;

                // Check if this is the trigger output
                if (rule.trigger_output_id == outId) {
                    bool shouldFire = (rule.trigger_on_rise && riseEdge) ||
                                      (rule.trigger_on_fall && fallEdge);
                    if (shouldFire) {
                        _applyAction(rule, engine);
                        if (rule.release_auto_ms > 0)
                            _actionTime[ruleId] = now;
                        else
                            _actionTime.erase(ruleId);
                    }
                }

                // Check if this is the release output
                if (!rule.release_output_id.isEmpty() &&
                    rule.release_output_id == outId) {
                    bool releaseEdge = (rule.release_on_rise  && riseEdge) ||
                                       (!rule.release_on_rise && fallEdge);
                    if (releaseEdge) {
                        _reverseAction(rule, engine);
                        _actionTime.erase(ruleId);
                    }
                }
            }
        }

        // --- Auto-release timers ---
        for (auto& [ruleId, rule] : _rules) {
            if (!rule.enabled || rule.release_auto_ms == 0) continue;
            auto it = _actionTime.find(ruleId);
            if (it == _actionTime.end()) continue;
            if (now - it->second >= rule.release_auto_ms) {
                _reverseAction(rule, engine);
                _actionTime.erase(it);
            }
        }
    }

private:
    std::map<String, OutputRule>  _rules;
    std::map<String, bool>        _prevStates;
    std::map<String, uint32_t>    _actionTime;   // ruleId -> when action was applied
    bool _initialized = false;

    // ------------------------------------------------------------------
    // HELPERS
    // ------------------------------------------------------------------

    void _applyAction(const OutputRule& rule,
                      BehavioralOutput::BehaviorEngine& engine) {
        String a = rule.action;
        a.toLowerCase();

        if (a == "on") {
            BehavioralOutput::BehaviorConfig cfg;
            cfg.type = BehavioralOutput::BehaviorType::STEADY;
            cfg.targetValue = 255;
            engine.setBehavior(rule.action_output_id, cfg);

        } else if (a == "off") {
            engine.deactivateOutput(rule.action_output_id);

        } else if (a == "toggle") {
            auto* out = engine.getOutput(rule.action_output_id);
            if (out) {
                if (out->isActive && out->currentState) {
                    engine.deactivateOutput(rule.action_output_id);
                } else {
                    BehavioralOutput::BehaviorConfig cfg;
                    cfg.type = BehavioralOutput::BehaviorType::STEADY;
                    cfg.targetValue = 255;
                    engine.setBehavior(rule.action_output_id, cfg);
                }
            }

        } else if (a == "flash") {
            BehavioralOutput::BehaviorConfig cfg;
            cfg.type = BehavioralOutput::BehaviorType::FLASH;
            cfg.targetValue = 255;
            cfg.period_ms = 500;
            cfg.dutyCycle = 50;
            engine.setBehavior(rule.action_output_id, cfg);

        } else if (a == "scene_activate") {
            engine.activateScene(rule.action_scene_id);

        } else if (a == "scene_deactivate") {
            engine.deactivateScene(rule.action_scene_id);
        }

        const char* target = rule.action_output_id.isEmpty()
            ? rule.action_scene_id.c_str()
            : rule.action_output_id.c_str();
        Serial.printf("[OutputRule] '%s' fired: %s '%s'\n",
                      rule.name.c_str(), rule.action.c_str(), target);
    }

    void _reverseAction(const OutputRule& rule,
                        BehavioralOutput::BehaviorEngine& engine) {
        String a = rule.action;
        a.toLowerCase();

        if (a == "on" || a == "flash") {
            engine.deactivateOutput(rule.action_output_id);

        } else if (a == "off") {
            BehavioralOutput::BehaviorConfig cfg;
            cfg.type = BehavioralOutput::BehaviorType::STEADY;
            cfg.targetValue = 255;
            engine.setBehavior(rule.action_output_id, cfg);

        } else if (a == "scene_activate") {
            engine.deactivateScene(rule.action_scene_id);

        } else if (a == "scene_deactivate") {
            engine.activateScene(rule.action_scene_id);
        }
        // "toggle" has no clean inverse – intentionally skipped

        Serial.printf("[OutputRule] '%s' released\n", rule.name.c_str());
    }
};
