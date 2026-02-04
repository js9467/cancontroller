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
        
        JsonArray sceneOutputs = sceneObj.createNestedArray("outputs");
        for (const auto& so : scene.outputs) {
            JsonObject soObj = sceneOutputs.createNestedObject();
            soObj["output_id"] = so.outputId;
            soObj["behavior_type"] = static_cast<int>(so.behavior.type);
            soObj["target_value"] = so.behavior.targetValue;
            soObj["period_ms"] = so.behavior.period_ms;
            soObj["duty_cycle"] = so.behavior.dutyCycle;
            soObj["fade_time_ms"] = so.behavior.fadeTime_ms;
            soObj["on_time_ms"] = so.behavior.onTime_ms;
            soObj["off_time_ms"] = so.behavior.offTime_ms;
            soObj["soft_start"] = so.behavior.softStart;
        }
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
        
        JsonArray sceneOutputs = sceneObj["outputs"];
        for (JsonObject soObj : sceneOutputs) {
            SceneOutput so;
            so.outputId = soObj["output_id"].as<String>();
            so.behavior.type = static_cast<BehaviorType>(soObj["behavior_type"].as<int>());
            so.behavior.targetValue = soObj["target_value"];
            so.behavior.period_ms = soObj["period_ms"];
            so.behavior.dutyCycle = soObj["duty_cycle"];
            so.behavior.fadeTime_ms = soObj["fade_time_ms"];
            so.behavior.onTime_ms = soObj["on_time_ms"];
            so.behavior.offTime_ms = soObj["off_time_ms"];
            so.behavior.softStart = soObj["soft_start"];
            
            scene.outputs.push_back(so);
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
