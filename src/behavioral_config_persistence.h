#pragma once

#include "output_behavior_engine.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

/**
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  BEHAVIORAL OUTPUT CONFIGURATION PERSISTENCE                              ║
 * ║                                                                           ║
 * ║  Save and load user-defined outputs and scenes to/from LittleFS          ║
 * ║  Prevents configuration loss during firmware updates                     ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

namespace BehavioralOutput {

static const char* BEHAVIORAL_CONFIG_FILE = "/behavioral_config.json";

// ═══════════════════════════════════════════════════════════════════════════
// SAVE CONFIGURATION TO LITTLEFS
// ═══════════════════════════════════════════════════════════════════════════

inline bool saveBehavioralConfig(BehaviorEngine& engine) {
    Serial.println("[Behavioral Persistence] Saving configuration to LittleFS...");
    
    DynamicJsonDocument doc(32768); // 32KB buffer
    
    // Save outputs
    JsonArray outputsArray = doc.createNestedArray("outputs");
    const auto& outputs = engine.getOutputs();
    
    for (const auto& [id, output] : outputs) {
        JsonObject outObj = outputsArray.createNestedObject();
        outObj["id"] = output.id;
        outObj["name"] = output.name;
        outObj["description"] = output.description;
        outObj["cell_address"] = output.cellAddress;
        outObj["output_number"] = output.outputNumber;
    }
    
    // Save scenes
    JsonArray scenesArray = doc.createNestedArray("scenes");
    const auto& scenes = engine.getScenes();
    
    for (const auto& [id, scene] : scenes) {
        JsonObject sceneObj = scenesArray.createNestedObject();
        sceneObj["id"] = scene.id;
        sceneObj["name"] = scene.name;
        sceneObj["description"] = scene.description;
        sceneObj["exclusive"] = scene.exclusive;
        sceneObj["duration_ms"] = scene.duration_ms;
        sceneObj["priority"] = scene.priority;
        
        JsonArray sceneOutputs = sceneObj.createNestedArray("outputs");
        for (const auto& so : scene.outputs) {
            JsonObject soObj = sceneOutputs.createNestedObject();
            soObj["output_id"] = so.outputId;
            soObj["action"] = so.action;
            soObj["behavior_type"] = static_cast<int>(so.behavior.type);
            soObj["target_value"] = so.behavior.targetValue;
            soObj["period_ms"] = so.behavior.period_ms;
            soObj["duty_cycle"] = so.behavior.dutyCycle;
            soObj["fade_time_ms"] = so.behavior.fadeTime_ms;
            soObj["on_time_ms"] = so.behavior.onTime_ms;
            soObj["off_time_ms"] = so.behavior.offTime_ms;
            soObj["soft_start"] = so.behavior.softStart;
            soObj["duration_ms"] = so.behavior.duration_ms;
            soObj["priority"] = so.behavior.priority;
            soObj["auto_off"] = so.behavior.autoOff;
        }

        JsonArray canFrames = sceneObj.createNestedArray("can_frames");
        for (const auto& frame : scene.can_frames) {
            JsonObject fObj = canFrames.createNestedObject();
            fObj["enabled"] = frame.enabled;
            fObj["pgn"] = frame.pgn;
            fObj["priority"] = frame.priority;
            fObj["source"] = frame.source_address;
            fObj["destination"] = frame.destination_address;
            fObj["length"] = frame.length;
            JsonArray dataArr = fObj.createNestedArray("data");
            for (size_t i = 0; i < frame.length && i < frame.data.size(); ++i) {
                dataArr.add(frame.data[i]);
            }
        }

        JsonArray iboxActions = sceneObj.createNestedArray("infinitybox_actions");
        for (const auto& action : scene.infinitybox_actions) {
            JsonObject aObj = iboxActions.createNestedObject();
            aObj["function"] = action.function_name;
            aObj["behavior"] = action.behavior;
            aObj["level"] = action.level;
            aObj["on_ms"] = action.on_ms;
            aObj["off_ms"] = action.off_ms;
            aObj["duration_ms"] = action.duration_ms;
            aObj["release_on_deactivate"] = action.release_on_deactivate;
        }

        JsonObject suspObj = sceneObj.createNestedObject("suspension");
        suspObj["enabled"] = scene.suspension.enabled;
        suspObj["front_left"] = scene.suspension.front_left;
        suspObj["front_right"] = scene.suspension.front_right;
        suspObj["rear_left"] = scene.suspension.rear_left;
        suspObj["rear_right"] = scene.suspension.rear_right;
        suspObj["calibration_active"] = scene.suspension.calibration_active;
    }
    
    // Write to file
    File file = LittleFS.open(BEHAVIORAL_CONFIG_FILE, "w");
    if (!file) {
        Serial.println("[Behavioral Persistence] ERROR: Failed to open config file for writing");
        return false;
    }
    
    size_t bytesWritten = serializeJson(doc, file);
    file.close();
    
    Serial.printf("[Behavioral Persistence] Saved %d outputs and %d scenes (%d bytes)\n", 
        outputs.size(), scenes.size(), bytesWritten);
    
    return bytesWritten > 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// LOAD CONFIGURATION FROM LITTLEFS
// ═══════════════════════════════════════════════════════════════════════════

inline bool loadBehavioralConfig(BehaviorEngine& engine) {
    if (!LittleFS.exists(BEHAVIORAL_CONFIG_FILE)) {
        Serial.println("[Behavioral Persistence] No saved configuration found");
        return false;
    }
    
    Serial.println("[Behavioral Persistence] Loading configuration from LittleFS...");
    
    File file = LittleFS.open(BEHAVIORAL_CONFIG_FILE, "r");
    if (!file) {
        Serial.println("[Behavioral Persistence] ERROR: Failed to open config file for reading");
        return false;
    }
    
    DynamicJsonDocument doc(32768);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("[Behavioral Persistence] ERROR: Failed to parse config file: %s\n", error.c_str());
        return false;
    }
    
    // Load outputs
    JsonArray outputsArray = doc["outputs"];
    int outputCount = 0;
    
    for (JsonObject outObj : outputsArray) {
        OutputChannel output;
        output.id = outObj["id"].as<String>();
        output.name = outObj["name"].as<String>();
        output.description = outObj["description"].as<String>();
        output.cellAddress = outObj["cell_address"];
        output.outputNumber = outObj["output_number"];
        
        engine.addOutput(output);
        outputCount++;
    }
    
    // Load scenes
    JsonArray scenesArray = doc["scenes"];
    int sceneCount = 0;
    
    for (JsonObject sceneObj : scenesArray) {
        Scene scene;
        scene.id = sceneObj["id"].as<String>();
        scene.name = sceneObj["name"].as<String>();
        scene.description = sceneObj["description"].as<String>();
        scene.exclusive = sceneObj["exclusive"];
        scene.duration_ms = sceneObj["duration_ms"] | 0;
        scene.priority = sceneObj["priority"] | 100;
        
        JsonArray sceneOutputs = sceneObj["outputs"];
        for (JsonObject soObj : sceneOutputs) {
            SceneOutput so;
            so.outputId = soObj["output_id"].as<String>();
            so.action = soObj["action"] | "behavior";
            so.behavior.type = static_cast<BehaviorType>(soObj["behavior_type"].as<int>());
            so.behavior.targetValue = soObj["target_value"];
            so.behavior.period_ms = soObj["period_ms"];
            so.behavior.dutyCycle = soObj["duty_cycle"];
            so.behavior.fadeTime_ms = soObj["fade_time_ms"];
            so.behavior.onTime_ms = soObj["on_time_ms"];
            so.behavior.offTime_ms = soObj["off_time_ms"];
            so.behavior.softStart = soObj["soft_start"];
            so.behavior.duration_ms = soObj["duration_ms"] | 0;
            so.behavior.priority = soObj["priority"] | 100;
            so.behavior.autoOff = soObj["auto_off"] | true;
            
            scene.outputs.push_back(so);
        }

        JsonArray canFrames = sceneObj["can_frames"];
        for (JsonObject fObj : canFrames) {
            SceneCanFrame frame;
            frame.enabled = fObj["enabled"] | true;
            frame.pgn = fObj["pgn"] | 0x00FF00;
            frame.priority = fObj["priority"] | 6;
            frame.source_address = fObj["source"] | 0xF9;
            frame.destination_address = fObj["destination"] | 0xFF;
            JsonArray dataArr = fObj["data"];
            size_t idx = 0;
            for (JsonVariant v : dataArr) {
                if (idx >= frame.data.size()) break;
                frame.data[idx++] = v.as<uint8_t>();
            }
            frame.length = fObj["length"] | static_cast<uint8_t>(idx);
            scene.can_frames.push_back(frame);
        }

        JsonArray iboxActions = sceneObj["infinitybox_actions"];
        for (JsonObject aObj : iboxActions) {
            SceneInfinityboxAction action;
            action.function_name = aObj["function"].as<String>();
            action.behavior = aObj["behavior"] | "on";
            action.level = aObj["level"] | 100;
            action.on_ms = aObj["on_ms"] | 500;
            action.off_ms = aObj["off_ms"] | 500;
            action.duration_ms = aObj["duration_ms"] | 0;
            action.release_on_deactivate = aObj["release_on_deactivate"] | true;
            scene.infinitybox_actions.push_back(action);
        }

        JsonObject suspObj = sceneObj["suspension"];
        if (!suspObj.isNull()) {
            scene.suspension.enabled = suspObj["enabled"] | false;
            scene.suspension.front_left = suspObj["front_left"] | 0;
            scene.suspension.front_right = suspObj["front_right"] | 0;
            scene.suspension.rear_left = suspObj["rear_left"] | 0;
            scene.suspension.rear_right = suspObj["rear_right"] | 0;
            scene.suspension.calibration_active = suspObj["calibration_active"] | false;
        }
        
        engine.addScene(scene);
        sceneCount++;
    }
    
    Serial.printf("[Behavioral Persistence] Loaded %d outputs and %d scenes from persistent storage\n", 
        outputCount, sceneCount);
    
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// AUTO-SAVE ON API CHANGES
// ═══════════════════════════════════════════════════════════════════════════

inline void enableAutoSave(BehaviorEngine& engine) {
    // This would require modifying BehaviorEngine to call saveBehavioralConfig
    // after addOutput/addScene/updateOutput/updateScene/deleteOutput/deleteScene
    // For now, we'll manually save after API calls
    Serial.println("[Behavioral Persistence] Auto-save enabled (manual trigger required)");
}

} // namespace BehavioralOutput
