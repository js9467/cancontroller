#pragma once
#ifndef BEHAVIORAL_OUTPUT_INTEGRATION_H
#define BEHAVIORAL_OUTPUT_INTEGRATION_H

/**
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  BEHAVIORAL OUTPUT SYSTEM - SIMPLIFIED INTEGRATION                        ║
 * ║                                                                           ║
 * ║  User-centric design:                                                    ║
 * ║  • Define outputs dynamically via web UI (no hardcoded presets)          ║
 * ║  • Configure behaviors per-button on the button creation screen          ║
 * ║  • Build complex scenes with the scene builder tool                      ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "output_behavior_engine.h"
#include "output_frame_synthesizer.h"
#include "behavioral_output_api.h"
#include "behavioral_output_ui.h"
#include "behavioral_config_persistence.h"
#include "can_manager.h"
#include "ipm1_can_library.h"
#include <ESPAsyncWebServer.h>

using namespace BehavioralOutput;

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL INSTANCES
// ═══════════════════════════════════════════════════════════════════════════

inline BehaviorEngine behaviorEngine;
inline PowercellSynthesizer* powercellSynthesizer = nullptr;
inline BehavioralOutputAPI* outputAPI = nullptr;

// Forward declarations
inline void loadInfinityBoxDefaults();
inline void loadDefaultScenes();

// ═══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

inline void initBehavioralOutputSystem(AsyncWebServer* webServer) {
    Serial.println("╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║  BEHAVIORAL OUTPUT SYSTEM - INITIALIZING                      ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝");
    
    // Create frame synthesizer with CAN send callback
    Serial.println("[Behavioral] Creating PowercellSynthesizer...");
    powercellSynthesizer = new PowercellSynthesizer(
        &behaviorEngine,
        [](uint32_t pgn, uint8_t* data) {
            // Send POWERCELL CAN frame via existing CanManager
            constexpr uint8_t priority = 6;
            const uint8_t source_addr = Ipm1Can::kSourceAddress;
            CanManager::instance().sendJ1939Pgn(priority, pgn, source_addr, data);
        }
    );
    Serial.println("[Behavioral] ✓ PowercellSynthesizer created");
    
    // Configure update rates
    Serial.println("[Behavioral] Configuring update rates...");
    behaviorEngine.setUpdateInterval(20);  // 50Hz behavior engine
    powercellSynthesizer->setTransmitInterval(50);  // 20Hz CAN transmission
    Serial.println("[Behavioral] ✓ Update rates: 50Hz engine, 20Hz transmission");
    
    // Register REST API endpoints
    if (webServer) {
        outputAPI = new BehavioralOutputAPI(webServer, &behaviorEngine);
        outputAPI->registerEndpoints();
        
        // Serve the main UI
        webServer->on("/behavioral", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send_P(200, "text/html", BEHAVIORAL_OUTPUT_UI);
        });
    }
    
    // Try to load from persistent storage first
    bool loaded = loadBehavioralConfig(behaviorEngine);
    size_t outputCount = behaviorEngine.getOutputs().size();
    size_t sceneCount = behaviorEngine.getScenes().size();
    
    if (!loaded) {
        // No saved config - load InfinityBox standard outputs
        Serial.println("[Behavioral Output] No saved config, loading InfinityBox defaults...");
        loadInfinityBoxDefaults();
        loadDefaultScenes();
        outputCount = behaviorEngine.getOutputs().size();
        sceneCount = behaviorEngine.getScenes().size();
    } else {
        if (outputCount == 0) {
            Serial.println("[Behavioral Output] Saved config contained zero outputs. Restoring InfinityBox defaults...");
            loadInfinityBoxDefaults();
            outputCount = behaviorEngine.getOutputs().size();
        }
        if (sceneCount == 0) {
            Serial.println("[Behavioral Output] Saved config had no scenes. Restoring default scenes...");
            loadDefaultScenes();
            sceneCount = behaviorEngine.getScenes().size();
        }
        Serial.printf("[Behavioral Output] Configuration ready (%u outputs, %u scenes)\n", 
                     static_cast<unsigned>(outputCount), 
                     static_cast<unsigned>(sceneCount));
    }
    
    Serial.println("[Behavioral Output] System initialized");
    Serial.println("[Behavioral Output] Visit /behavioral to view/modify outputs and scenes");
}

// ═══════════════════════════════════════════════════════════════════════════
// INFINITYBOX STANDARD OUTPUT DEFINITIONS
// User can modify these via /behavioral web interface
// ═══════════════════════════════════════════════════════════════════════════

inline void loadInfinityBoxDefaults() {
    Serial.println("[Behavioral Output] Loading InfinityBox IPM1 standard outputs...");
    
    // ═══════════════════════════════════════════════════════════════════════
    // POWERCELL FRONT (Cell 1) - Based on IPM1 System Assignments
    // ═══════════════════════════════════════════════════════════════════════
    
    // ───────────────────────────────────────────────────────────────────────
    // TURN SIGNALS FRONT - PC Front Outputs 1-2
    // ───────────────────────────────────────────────────────────────────────
    OutputChannel leftTurnFront;
    leftTurnFront.id = "left_turn_front";
    leftTurnFront.name = "Left Turn Signal Front";
    leftTurnFront.description = "Driver side front turn indicator";
    leftTurnFront.cellAddress = 1;
    leftTurnFront.outputNumber = 1;
    behaviorEngine.addOutput(leftTurnFront);
    
    OutputChannel rightTurnFront;
    rightTurnFront.id = "right_turn_front";
    rightTurnFront.name = "Right Turn Signal Front";
    rightTurnFront.description = "Passenger side front turn indicator";
    rightTurnFront.cellAddress = 1;
    rightTurnFront.outputNumber = 2;
    behaviorEngine.addOutput(rightTurnFront);
    
    // ───────────────────────────────────────────────────────────────────────
    // POWERTRAIN - PC Front Outputs 3-4
    // ───────────────────────────────────────────────────────────────────────
    OutputChannel ignition;
    ignition.id = "ignition";
    ignition.name = "Ignition";
    ignition.description = "Engine ignition power";
    ignition.cellAddress = 1;
    ignition.outputNumber = 3;
    behaviorEngine.addOutput(ignition);
    
    OutputChannel starter;
    starter.id = "starter";
    starter.name = "Starter";
    starter.description = "Engine starter motor";
    starter.cellAddress = 1;
    starter.outputNumber = 4;
    behaviorEngine.addOutput(starter);
    
    // ───────────────────────────────────────────────────────────────────────
    // HEADLIGHTS & PARKING FRONT - PC Front Outputs 5-6
    // ───────────────────────────────────────────────────────────────────────
    OutputChannel headlights;
    headlights.id = "headlights";
    headlights.name = "Headlights";
    headlights.description = "Front headlights";
    headlights.cellAddress = 1;
    headlights.outputNumber = 5;
    behaviorEngine.addOutput(headlights);
    
    OutputChannel parkingFront;
    parkingFront.id = "parking_front";
    parkingFront.name = "Parking Lights Front";
    parkingFront.description = "Front parking/marker lights";
    parkingFront.cellAddress = 1;
    parkingFront.outputNumber = 6;
    behaviorEngine.addOutput(parkingFront);
    
    // ───────────────────────────────────────────────────────────────────────
    // HIGH BEAMS & HORN - PC Front Outputs 7, 9
    // ───────────────────────────────────────────────────────────────────────
    OutputChannel highBeams;
    highBeams.id = "high_beams";
    highBeams.name = "High Beams";
    highBeams.description = "High beam headlights";
    highBeams.cellAddress = 1;
    highBeams.outputNumber = 7;
    behaviorEngine.addOutput(highBeams);
    
    OutputChannel horn;
    horn.id = "horn";
    horn.name = "Horn";
    horn.description = "Vehicle horn";
    horn.cellAddress = 1;
    horn.outputNumber = 9;
    behaviorEngine.addOutput(horn);
    
    // ───────────────────────────────────────────────────────────────────────
    // COOLING FAN - PC Front Output 10
    // ───────────────────────────────────────────────────────────────────────
    OutputChannel coolingFan;
    coolingFan.id = "cooling_fan";
    coolingFan.name = "Cooling Fan";
    coolingFan.description = "Engine cooling fan";
    coolingFan.cellAddress = 1;
    coolingFan.outputNumber = 10;
    behaviorEngine.addOutput(coolingFan);
    
    // ═══════════════════════════════════════════════════════════════════════
    // POWERCELL REAR (Cell 2) - Based on IPM1 System Assignments
    // ═══════════════════════════════════════════════════════════════════════
    
    // ───────────────────────────────────────────────────────────────────────
    // TURN SIGNALS REAR - PC Rear Outputs 1-2
    // ───────────────────────────────────────────────────────────────────────
    OutputChannel leftTurnRear;
    leftTurnRear.id = "left_turn_rear";
    leftTurnRear.name = "Left Turn Signal Rear";
    leftTurnRear.description = "Driver side rear turn indicator";
    leftTurnRear.cellAddress = 2;
    leftTurnRear.outputNumber = 1;
    behaviorEngine.addOutput(leftTurnRear);
    
    OutputChannel rightTurnRear;
    rightTurnRear.id = "right_turn_rear";
    rightTurnRear.name = "Right Turn Signal Rear";
    rightTurnRear.description = "Passenger side rear turn indicator";
    rightTurnRear.cellAddress = 2;
    rightTurnRear.outputNumber = 2;
    behaviorEngine.addOutput(rightTurnRear);
    
    // ───────────────────────────────────────────────────────────────────────
    // BRAKE & INTERIOR - PC Rear Outputs 3-4
    // ───────────────────────────────────────────────────────────────────────
    OutputChannel brakeLights;
    brakeLights.id = "brake_lights";
    brakeLights.name = "Brake Lights";
    brakeLights.description = "Rear brake lights";
    brakeLights.cellAddress = 2;
    brakeLights.outputNumber = 3;
    behaviorEngine.addOutput(brakeLights);
    
    OutputChannel interiorLights;
    interiorLights.id = "interior_lights";
    interiorLights.name = "Interior Lights";
    interiorLights.description = "Cabin interior lighting";
    interiorLights.cellAddress = 2;
    interiorLights.outputNumber = 4;
    behaviorEngine.addOutput(interiorLights);
    
    // ───────────────────────────────────────────────────────────────────────
    // BACKUP & PARKING REAR - PC Rear Outputs 5-6
    // ───────────────────────────────────────────────────────────────────────
    OutputChannel backupLights;
    backupLights.id = "backup_lights";
    backupLights.name = "Backup Lights";
    backupLights.description = "Reverse/backup lights";
    backupLights.cellAddress = 2;
    backupLights.outputNumber = 5;
    behaviorEngine.addOutput(backupLights);
    
    OutputChannel parkingRear;
    parkingRear.id = "parking_rear";
    parkingRear.name = "Parking Lights Rear";
    parkingRear.description = "Rear parking/marker lights";
    parkingRear.cellAddress = 2;
    parkingRear.outputNumber = 6;
    behaviorEngine.addOutput(parkingRear);
    
    // ───────────────────────────────────────────────────────────────────────
    // FUEL PUMP - PC Rear Output 10
    // ───────────────────────────────────────────────────────────────────────
    OutputChannel fuelPump;
    fuelPump.id = "fuel_pump";
    fuelPump.name = "Fuel Pump";
    fuelPump.description = "Electric fuel pump";
    fuelPump.cellAddress = 2;
    fuelPump.outputNumber = 10;
    behaviorEngine.addOutput(fuelPump);
    
    Serial.printf("[Behavioral Output] Loaded %d InfinityBox IPM1 standard outputs\n", 17);
}

// ═══════════════════════════════════════════════════════════════════════════
// DEFAULT SCENES
// ═══════════════════════════════════════════════════════════════════════════

inline void loadDefaultScenes() {
    Serial.println("[Behavioral Output] Loading default scenes...");
    
    // ───────────────────────────────────────────────────────────────────────
    // SCENE: Left Turn Signal (Front + Rear)
    // ───────────────────────────────────────────────────────────────────────
    Scene leftTurnScene;
    leftTurnScene.id = "left_turn";
    leftTurnScene.name = "Left Turn Signal";
    leftTurnScene.description = "Flash left turn indicators at 1Hz";
    leftTurnScene.exclusive = false;
    
    SceneOutput leftFront;
    leftFront.outputId = "left_turn_front";
    leftFront.behavior.type = BehaviorType::FLASH;
    leftFront.behavior.targetValue = 255;
    leftFront.behavior.period_ms = 1000;
    leftFront.behavior.dutyCycle = 50;
    leftTurnScene.outputs.push_back(leftFront);
    
    SceneOutput leftRear;
    leftRear.outputId = "left_turn_rear";
    leftRear.behavior.type = BehaviorType::FLASH;
    leftRear.behavior.targetValue = 255;
    leftRear.behavior.period_ms = 1000;
    leftRear.behavior.dutyCycle = 50;
    leftTurnScene.outputs.push_back(leftRear);
    behaviorEngine.addScene(leftTurnScene);
    
    // ───────────────────────────────────────────────────────────────────────
    // SCENE: Right Turn Signal (Front + Rear)
    // ───────────────────────────────────────────────────────────────────────
    Scene rightTurnScene;
    rightTurnScene.id = "right_turn";
    rightTurnScene.name = "Right Turn Signal";
    rightTurnScene.description = "Flash right turn indicators at 1Hz";
    rightTurnScene.exclusive = false;
    
    SceneOutput rightFront;
    rightFront.outputId = "right_turn_front";
    rightFront.behavior.type = BehaviorType::FLASH;
    rightFront.behavior.targetValue = 255;
    rightFront.behavior.period_ms = 1000;
    rightFront.behavior.dutyCycle = 50;
    rightTurnScene.outputs.push_back(rightFront);
    
    SceneOutput rightRear;
    rightRear.outputId = "right_turn_rear";
    rightRear.behavior.type = BehaviorType::FLASH;
    rightRear.behavior.targetValue = 255;
    rightRear.behavior.period_ms = 1000;
    rightRear.behavior.dutyCycle = 50;
    rightTurnScene.outputs.push_back(rightRear);
    behaviorEngine.addScene(rightTurnScene);
    
    // ───────────────────────────────────────────────────────────────────────
    // SCENE: Hazard Flashers (4-Way)
    // ───────────────────────────────────────────────────────────────────────
    Scene hazardScene;
    hazardScene.id = "hazard";
    hazardScene.name = "Hazard Flashers";
    hazardScene.description = "Flash all turn signals simultaneously";
    hazardScene.exclusive = false;
    
    hazardScene.outputs.push_back(leftFront);
    hazardScene.outputs.push_back(rightFront);
    hazardScene.outputs.push_back(leftRear);
    hazardScene.outputs.push_back(rightRear);
    behaviorEngine.addScene(hazardScene);
    
    // ───────────────────────────────────────────────────────────────────────
    // SCENE: Brake Lights
    // ───────────────────────────────────────────────────────────────────────
    Scene brakeScene;
    brakeScene.id = "brake_on";
    brakeScene.name = "Brake Lights";
    brakeScene.description = "Activate brake lights (steady)";
    brakeScene.exclusive = false;
    
    SceneOutput brakeOut;
    brakeOut.outputId = "brake_lights";
    brakeOut.behavior.type = BehaviorType::STEADY;
    brakeOut.behavior.targetValue = 255;
    brakeScene.outputs.push_back(brakeOut);
    behaviorEngine.addScene(brakeScene);
    
    Serial.printf("[Behavioral Output] Loaded %d default scenes\n", 
        behaviorEngine.getAllScenes().size());
}

// ═══════════════════════════════════════════════════════════════════════════// ═══════════════════════════════════════════════════════════════════════════
// MAIN LOOP INTEGRATION
// ═══════════════════════════════════════════════════════════════════════════

inline void updateBehavioralOutputSystem() {
    static uint32_t last_debug_ms = 0;
    static uint32_t update_count = 0;
    
    update_count++;
    
    // Debug every 5 seconds to confirm update loop is running
    uint32_t now = millis();
    if (now - last_debug_ms >= 5000) {
        Serial.printf("[Behavioral] Update loop running: %lu updates in 5s\n", update_count);
        update_count = 0;
        last_debug_ms = now;
    }
    
    // Update behavior engine (evaluates all behaviors)
    behaviorEngine.update();
    
    // Synthesize and transmit POWERCELL frames
    if (powercellSynthesizer) {
        powercellSynthesizer->update();
    }
}

#endif // BEHAVIORAL_OUTPUT_INTEGRATION_H
