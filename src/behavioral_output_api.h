#pragma once

#include <Arduino.h>
#include <ctype.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "output_behavior_engine.h"
#include "output_frame_synthesizer.h"
#include "can_manager.h"
#include "behavioral_config_persistence.h"

/**
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  BEHAVIORAL OUTPUT REST API                                               ║
 * ║                                                                           ║
 * ║  Provides HTTP endpoints for runtime control of behavioral outputs       ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

namespace BehavioralOutput {

class BehavioralOutputAPI {
public:
    BehavioralOutputAPI(AsyncWebServer* server, BehaviorEngine* engine)
        : _server(server), _engine(engine) {}
    
    void registerEndpoints() {
        // ═══════════════════════════════════════════════════════════════════
        // DEBUG TEST ENDPOINT - Direct behavioral engine activation
        // ═══════════════════════════════════════════════════════════════════
        
        // GET /api/test/flash?output=left_turn_front - Test flash behavior directly
        _server->on("/api/test/flash", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String outputId = "left_turn_front";
            if (request->hasParam("output")) {
                outputId = request->getParam("output")->value();
            }
            
            BehaviorConfig behavior;
            behavior.type = BehaviorType::FLASH;
            behavior.targetValue = 255;
            behavior.onTime_ms = 500;
            behavior.offTime_ms = 500;
            behavior.period_ms = 1000;
            behavior.dutyCycle = 50;
            
            bool success = _engine->setBehavior(outputId, behavior);
            
            String response = "{\"success\":" + String(success ? "true" : "false") + 
                            ",\"output\":\"" + outputId + "\"" +
                            ",\"message\":\"Direct flash activation test\"}";
            request->send(200, "application/json", response);
        });
        
        // GET /api/test/steady?output=left_turn_front - Test steady ON directly
        _server->on("/api/test/steady", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String outputId = "left_turn_front";
            if (request->hasParam("output")) {
                outputId = request->getParam("output")->value();
            }
            
            BehaviorConfig behavior;
            behavior.type = BehaviorType::STEADY;
            behavior.targetValue = 255;
            
            bool success = _engine->setBehavior(outputId, behavior);
            
            String response = "{\"success\":" + String(success ? "true" : "false") + 
                            ",\"output\":\"" + outputId + "\"" +
                            ",\"message\":\"Direct steady ON test\"}";
            request->send(200, "application/json", response);
        });
        
        // GET /api/test/off?output=left_turn_front - Deactivate output
        _server->on("/api/test/off", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String outputId = "left_turn_front";
            if (request->hasParam("output")) {
                outputId = request->getParam("output")->value();
            }
            
            bool success = _engine->deactivateOutput(outputId);
            
            String response = "{\"success\":" + String(success ? "true" : "false") + 
                            ",\"output\":\"" + outputId + "\"}";
            request->send(200, "application/json", response);
        });
        
        // ═══════════════════════════════════════════════════════════════════
        // OUTPUT ENDPOINTS
        // ═══════════════════════════════════════════════════════════════════
        
        // GET /api/outputs/live - Live state snapshot
        _server->on("/api/outputs/live", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String json = serializeOutputStates();
            request->send(200, "application/json", json);
        });

        // GET /api/outputs - List all outputs
        _server->on("/api/outputs", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String json = serializeOutputs();
            request->send(200, "application/json", json);
        });

        // POST /api/output/behavior/{id} - Set output behavior (preferred)
        _server->on("/api/output/behavior/*", HTTP_POST, [](AsyncWebServerRequest* request){},
            nullptr,
            [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
                handleSetBehavior(request, data, len);
            });

        // POST /api/output/deactivate/{id} - Stop output (preferred)
        _server->on("/api/output/deactivate/*", HTTP_POST, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = resolveOutputId(extractOutputId(path));

            bool success = _engine->deactivateOutput(id);
            if (success) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Output not found\"}");
            }
        });
        
        // POST /api/outputs - Create new output
        _server->on("/api/outputs", HTTP_POST, [](AsyncWebServerRequest* request){},
            nullptr,
            [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
                const String path = request->url();
                if (path.indexOf("/behavior") >= 0) {
                    handleSetBehavior(request, data, len);
                    return;
                }
                handleCreateOutput(request, data, len);
            });
        
        // GET /api/outputs/{id} - Get specific output
        _server->on("/api/outputs/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractOutputId(path);

            if (id == "state") {
                String json = serializeOutputStates();
                request->send(200, "application/json", json);
                return;
            }
            
            auto* output = _engine->getOutput(id);
            if (!output) {
                request->send(404, "application/json", "{\"error\":\"Output not found\"}");
                return;
            }
            
            String json = serializeOutput(*output);
            request->send(200, "application/json", json);
        });
        
        // DELETE /api/outputs/{id} - Delete output
        _server->on("/api/outputs/*", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractOutputId(path);
            
            _engine->removeOutput(id);
            saveBehavioralConfig(*_engine); // Auto-save
            request->send(200, "application/json", "{\"success\":true}");
        });
        
        // ═══════════════════════════════════════════════════════════════════
        // BEHAVIOR ENDPOINTS
        // ═══════════════════════════════════════════════════════════════════
        
        // POST /api/outputs/{id}/behavior - Set output behavior
        _server->on("/api/outputs/*/behavior", HTTP_POST, [](AsyncWebServerRequest* request){},
            nullptr,
            [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
                handleSetBehavior(request, data, len);
            });
        
        // POST /api/outputs/{id}/deactivate - Stop output
        _server->on("/api/outputs/*/deactivate", HTTP_POST, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = resolveOutputId(extractOutputId(path));
            
            bool success = _engine->deactivateOutput(id);
            if (success) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Output not found\"}");
            }
        });
        
        // GET /api/outputs/state - Get current state of all outputs
        _server->on("/api/outputs/state", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String json = serializeOutputStates();
            request->send(200, "application/json", json);
        });
        
        // POST /api/outputs/stop-all - Stop all outputs
        _server->on("/api/outputs/stop-all", HTTP_POST, [this](AsyncWebServerRequest* request) {
            const auto& outputs = _engine->getOutputs();
            for (const auto& [id, output] : outputs) {
                _engine->deactivateOutput(id);
            }
            request->send(200, "application/json", "{\"success\":true}");
        });
        
        // ═══════════════════════════════════════════════════════════════════
        // PATTERN ENDPOINTS
        // ═══════════════════════════════════════════════════════════════════
        
        // GET /api/patterns - List all patterns
        _server->on("/api/patterns", HTTP_GET, [this](AsyncWebServerRequest* request) {
            request->send(200, "application/json", "[]"); // TODO: Implement
        });
        
        // POST /api/patterns - Create pattern
        _server->on("/api/patterns", HTTP_POST, [](AsyncWebServerRequest* request){},
            nullptr,
            [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
                // TODO: Implement pattern creation
                request->send(200, "application/json", "{\"success\":true}");
            });
        
        // ═══════════════════════════════════════════════════════════════════
        // SCENE ENDPOINTS
        // ═══════════════════════════════════════════════════════════════════
        
        // GET /api/scenes - List all scenes
        _server->on("/api/scenes", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String json = serializeScenes();
            request->send(200, "application/json", json);
        });

        // POST /api/scenes - Create scene
        _server->on("/api/scenes", HTTP_POST, [](AsyncWebServerRequest* request){},
            nullptr,
            [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
                handleCreateScene(request, data, len);
            });

        // GET /api/scenes/activate/{id} - Activate scene (GET fallback)
        _server->on("/api/scenes/activate/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractSceneId(path);

            bool success = _engine->activateScene(id);
            if (success) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
            }
        });

        // GET /api/scene/activate/{id} - Activate scene (preferred)
        _server->on("/api/scene/activate/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractSceneId(path);

            bool success = _engine->activateScene(id);
            if (success) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
            }
        });
        
        // POST /api/scenes/activate/{id} - Activate scene
        _server->on("/api/scenes/activate/*", HTTP_POST, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractSceneId(path);
            
            bool success = _engine->activateScene(id);
            if (success) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
            }
        });

        // POST /api/scene/activate/{id} - Activate scene (preferred)
        _server->on("/api/scene/activate/*", HTTP_POST, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractSceneId(path);

            bool success = _engine->activateScene(id);
            if (success) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
            }
        });
        
        // POST /api/scenes/deactivate/{id} - Deactivate scene
        _server->on("/api/scenes/deactivate/*", HTTP_POST, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractSceneId(path);
            
            bool success = _engine->deactivateScene(id);
            if (success) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
            }
        });

        // POST /api/scene/deactivate/{id} - Deactivate scene (preferred)
        _server->on("/api/scene/deactivate/*", HTTP_POST, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractSceneId(path);

            bool success = _engine->deactivateScene(id);
            if (success) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
            }
        });

        // GET /api/scenes/deactivate/{id} - Deactivate scene (GET fallback)
        _server->on("/api/scenes/deactivate/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractSceneId(path);

            bool success = _engine->deactivateScene(id);
            if (success) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
            }
        });

        // GET /api/scene/deactivate/{id} - Deactivate scene (preferred)
        _server->on("/api/scene/deactivate/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractSceneId(path);

            bool success = _engine->deactivateScene(id);
            if (success) {
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
            }
        });

        // GET /api/scenes/{id} - Get full scene detail
        _server->on("/api/scenes/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            if (path.startsWith("/api/scenes/activate/")) {
                String id = extractSceneId(path);
                bool success = _engine->activateScene(id);
                if (success) {
                    request->send(200, "application/json", "{\"success\":true}");
                } else {
                    request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
                }
                return;
            }
            if (path.startsWith("/api/scenes/deactivate/")) {
                String id = extractSceneId(path);
                bool success = _engine->deactivateScene(id);
                if (success) {
                    request->send(200, "application/json", "{\"success\":true}");
                } else {
                    request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
                }
                return;
            }
            String id = extractSceneId(path);
            auto* scene = _engine->getScene(id);
            if (!scene) {
                request->send(404, "application/json", "{\"error\":\"Scene not found\"}");
                return;
            }
            String json = serializeSceneDetail(*scene);
            request->send(200, "application/json", json);
        });
        
        // DELETE /api/scenes/{id} - Delete scene
        _server->on("/api/scenes/*", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractSceneId(path);
            
            _engine->removeScene(id);
            saveBehavioralConfig(*_engine); // Auto-save
            request->send(200, "application/json", "{\"success\":true}");
        });
    }

private:
    AsyncWebServer* _server;
    BehaviorEngine* _engine;
    
    // ═══════════════════════════════════════════════════════════════════════
    // SERIALIZATION
    // ═══════════════════════════════════════════════════════════════════════
    
    String serializeOutputs() {
        DynamicJsonDocument doc(8192);
        JsonArray array = doc.to<JsonArray>();
        
        const auto& outputs = _engine->getOutputs();
        for (const auto& [id, output] : outputs) {
            JsonObject obj = array.createNestedObject();
            obj["id"] = output.id;
            obj["name"] = output.name;
            obj["cellAddress"] = output.cellAddress;
            obj["outputNumber"] = output.outputNumber;
            obj["desiredActive"] = output.isActive;
            obj["desiredValue"] = output.currentState ? 255 : 0;
            obj["description"] = output.description;
            obj["cellAddress"] = output.cellAddress;
            obj["outputNumber"] = output.outputNumber;
            obj["isActive"] = output.isActive;
            obj["currentValue"] = output.currentState ? 255 : 0;
            
            if (output.isActive) {
                JsonObject behavior = obj.createNestedObject("behavior");
                behavior["type"] = behaviorTypeToString(output.behavior.type);
                behavior["targetValue"] = output.behavior.targetValue;
                behavior["priority"] = output.behavior.priority;
            }
        }
        
        String result;
        serializeJson(doc, result);
        return result;
    }
    
    String serializeOutput(const OutputChannel& output) {
        DynamicJsonDocument doc(2048);
        JsonObject obj = doc.to<JsonObject>();
        
        obj["id"] = output.id;
        obj["name"] = output.name;
        obj["description"] = output.description;
        obj["cellAddress"] = output.cellAddress;
        obj["outputNumber"] = output.outputNumber;
        obj["isActive"] = output.isActive;
        obj["currentValue"] = output.currentState ? 255 : 0;
        
        JsonObject behavior = obj.createNestedObject("behavior");
        behavior["type"] = behaviorTypeToString(output.behavior.type);
        behavior["targetValue"] = output.behavior.targetValue;
        behavior["period_ms"] = output.behavior.period_ms;
        behavior["dutyCycle"] = output.behavior.dutyCycle;
        behavior["duration_ms"] = output.behavior.duration_ms;
        behavior["priority"] = output.behavior.priority;
        
        String result;
        serializeJson(doc, result);
        return result;
    }
    
    String serializeOutputStates() {
        DynamicJsonDocument doc(4096);
        JsonArray array = doc.to<JsonArray>();

        const uint32_t now = millis();
        
        const auto& outputs = _engine->getOutputs();
        for (const auto& [id, output] : outputs) {
            JsonObject obj = array.createNestedObject();
            obj["id"] = output.id;
            obj["name"] = output.name;
            obj["cellAddress"] = output.cellAddress;
            obj["outputNumber"] = output.outputNumber;
            obj["desiredActive"] = output.isActive;
            obj["desiredValue"] = output.currentState ? 255 : 0;

            const auto canState = CanManager::instance().getPowercellOutputState(output.cellAddress, output.outputNumber);
            const auto cellTelemetry = CanManager::instance().getPowercellCellTelemetry(output.cellAddress);
            const bool canFresh = canState.valid && (now - canState.last_seen_ms <= 1000);
            if (canFresh) {
                const float currentAmps = static_cast<float>(canState.current_raw) * 0.117f;
                obj["currentValue"] = canState.current_raw;
                obj["currentAmps"] = currentAmps;
                obj["isActive"] = canState.on || canState.current_raw > 0;
                obj["source"] = "can";
                obj["lastSeenMs"] = canState.last_seen_ms;
            } else {
                obj["currentValue"] = output.currentState ? 255 : 0;
                obj["isActive"] = output.isActive;
                obj["source"] = "engine";
            }

            const bool cellFresh = cellTelemetry.valid && (now - cellTelemetry.last_seen_ms <= 1000);
            if (cellFresh) {
                obj["cellVoltageRaw"] = cellTelemetry.voltage_raw;
                obj["cellVoltageVolts"] = static_cast<float>(cellTelemetry.voltage_raw) * 0.125f;
                obj["cellTemperatureC"] = cellTelemetry.temperature_c;
                obj["cellLastSeenMs"] = cellTelemetry.last_seen_ms;
            }
        }
        
        String result;
        serializeJson(doc, result);
        return result;
    }
    
    String serializeScenes() {
        DynamicJsonDocument doc(8192);
        JsonArray array = doc.to<JsonArray>();
        
        const auto& scenes = _engine->getScenes();
        for (const auto& [id, scene] : scenes) {
            JsonObject obj = array.createNestedObject();
            obj["id"] = scene.id;
            obj["name"] = scene.name;
            obj["description"] = scene.description;
            obj["isActive"] = scene.isActive;
            obj["outputCount"] = scene.outputs.size();
            obj["canCount"] = scene.can_frames.size();
            obj["infinityboxCount"] = scene.infinitybox_actions.size();
            obj["suspensionEnabled"] = scene.suspension.enabled;
            obj["duration_ms"] = scene.duration_ms;
            obj["priority"] = scene.priority;
            obj["exclusive"] = scene.exclusive;

            JsonArray outputs = obj.createNestedArray("outputs");
            for (const auto& so : scene.outputs) {
                JsonObject soObj = outputs.createNestedObject();
                soObj["output_id"] = so.outputId;
                soObj["action"] = so.action;
                soObj["behavior_type"] = behaviorTypeToString(so.behavior.type);
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

            JsonArray canFrames = obj.createNestedArray("can_frames");
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

            JsonArray iboxActions = obj.createNestedArray("infinitybox_actions");
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

            JsonObject susp = obj.createNestedObject("suspension");
            susp["enabled"] = scene.suspension.enabled;
            susp["front_left"] = scene.suspension.front_left;
            susp["front_right"] = scene.suspension.front_right;
            susp["rear_left"] = scene.suspension.rear_left;
            susp["rear_right"] = scene.suspension.rear_right;
            susp["calibration_active"] = scene.suspension.calibration_active;
        }
        
        String result;
        serializeJson(doc, result);
        return result;
    }

    String serializeSceneDetail(const Scene& scene) {
        DynamicJsonDocument doc(8192);
        JsonObject obj = doc.to<JsonObject>();
        obj["id"] = scene.id;
        obj["name"] = scene.name;
        obj["description"] = scene.description;
        obj["isActive"] = scene.isActive;
        obj["duration_ms"] = scene.duration_ms;
        obj["priority"] = scene.priority;
        obj["exclusive"] = scene.exclusive;

        JsonArray outputs = obj.createNestedArray("outputs");
        for (const auto& so : scene.outputs) {
            JsonObject soObj = outputs.createNestedObject();
            soObj["output_id"] = so.outputId;
            soObj["action"] = so.action;
            soObj["behavior_type"] = behaviorTypeToString(so.behavior.type);
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

        JsonArray canFrames = obj.createNestedArray("can_frames");
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

        JsonArray iboxActions = obj.createNestedArray("infinitybox_actions");
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

        JsonObject susp = obj.createNestedObject("suspension");
        susp["enabled"] = scene.suspension.enabled;
        susp["front_left"] = scene.suspension.front_left;
        susp["front_right"] = scene.suspension.front_right;
        susp["rear_left"] = scene.suspension.rear_left;
        susp["rear_right"] = scene.suspension.rear_right;
        susp["calibration_active"] = scene.suspension.calibration_active;

        String result;
        serializeJson(doc, result);
        return result;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // REQUEST HANDLERS
    // ═══════════════════════════════════════════════════════════════════════
    
    void handleCreateOutput(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, data, len);
        
        if (error) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        OutputChannel output;
        if (doc.containsKey("id")) {
            output.id = doc["id"].as<String>();
        } else {
            output.id = String("out_") + String(millis());
        }
        output.name = doc.containsKey("name") ? doc["name"].as<String>() : String("Unnamed Output");
        output.description = doc.containsKey("description") ? doc["description"].as<String>() : String("");
        output.cellAddress = doc.containsKey("cellAddress") ? doc["cellAddress"].as<uint8_t>() : 1;
        output.outputNumber = doc.containsKey("outputNumber") ? doc["outputNumber"].as<uint8_t>() : 1;

        if (output.cellAddress > 254) {
            output.cellAddress = 254;
        }
        if (output.cellAddress == 0) {
            if (output.outputNumber < 1) output.outputNumber = 1;
            if (output.outputNumber > 8) output.outputNumber = 8;
        } else {
            if (output.outputNumber < 1) output.outputNumber = 1;
            if (output.outputNumber > 10) output.outputNumber = 10;
        }
        
        _engine->addOutput(output);
        saveBehavioralConfig(*_engine); // Auto-save
        
        request->send(200, "application/json", "{\"success\":true,\"id\":\"" + output.id + "\"}");
    }
    
    void handleSetBehavior(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
        String path = request->url();
        String id = resolveOutputId(extractOutputId(path));
        
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, data, len);
        
        if (error) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        BehaviorConfig behavior;
        behavior.type = stringToBehaviorType(doc["type"] | "STEADY");
        behavior.targetValue = doc["targetValue"] | 255;
        behavior.period_ms = doc["period_ms"] | 1000;
        behavior.dutyCycle = doc["dutyCycle"] | 50;
        behavior.duration_ms = doc["duration_ms"] | 0;
        behavior.fadeTime_ms = doc["fadeTime_ms"] | 500;
        behavior.onTime_ms = doc["onTime_ms"] | 500;
        behavior.offTime_ms = doc["offTime_ms"] | 500;
        behavior.priority = doc["priority"] | 100;
        behavior.softStart = doc["softStart"] | false;
        
        bool success = _engine->setBehavior(id, behavior);
        
        if (success) {
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            request->send(404, "application/json", "{\"error\":\"Output not found\"}");
        }
    }
    
    void handleCreateScene(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, data, len);
        
        if (error) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        Scene scene;
        if (doc.containsKey("id")) {
            scene.id = doc["id"].as<String>();
        } else {
            scene.id = String("scene_") + String(millis());
        }
        scene.name = doc.containsKey("name") ? doc["name"].as<String>() : String("Unnamed Scene");
        scene.description = doc.containsKey("description") ? doc["description"].as<String>() : String("");
        scene.duration_ms = doc.containsKey("duration_ms") ? doc["duration_ms"].as<uint16_t>() : 0;
        scene.priority = doc.containsKey("priority") ? doc["priority"].as<uint8_t>() : 100;
        scene.exclusive = doc.containsKey("exclusive") ? doc["exclusive"].as<bool>() : false;
        
        // Parse scene outputs
        if (doc.containsKey("outputs")) {
            JsonArray outputsArray = doc["outputs"];
            for (JsonObject outObj : outputsArray) {
                SceneOutput so;
                so.outputId = outObj["output_id"].as<String>();
                so.action = outObj["action"] | "behavior";
                so.behavior.type = stringToBehaviorType(outObj["behavior_type"] | "STEADY");
                so.behavior.targetValue = outObj["target_value"] | 255;
                so.behavior.period_ms = outObj["period_ms"] | 1000;
                so.behavior.dutyCycle = outObj["duty_cycle"] | 50;
                so.behavior.fadeTime_ms = outObj["fade_time_ms"] | 500;
                so.behavior.onTime_ms = outObj["on_time_ms"] | 500;
                so.behavior.offTime_ms = outObj["off_time_ms"] | 500;
                so.behavior.softStart = outObj["soft_start"] | false;
                so.behavior.duration_ms = outObj["duration_ms"] | 0;
                so.behavior.priority = outObj["priority"] | 100;
                so.behavior.autoOff = outObj["auto_off"] | true;
                scene.outputs.push_back(so);
            }
        }

        if (doc.containsKey("can_frames")) {
            JsonArray framesArray = doc["can_frames"];
            for (JsonObject fObj : framesArray) {
                SceneCanFrame frame;
                frame.enabled = fObj["enabled"] | true;
                frame.pgn = fObj["pgn"] | 0x00FF00;
                frame.priority = fObj["priority"] | 6;
                frame.source_address = fObj["source"] | 0xF9;
                frame.destination_address = fObj["destination"] | 0xFF;
                JsonArray dataArray = fObj["data"];
                size_t idx = 0;
                for (JsonVariant v : dataArray) {
                    if (idx >= frame.data.size()) break;
                    frame.data[idx++] = v.as<uint8_t>();
                }
                frame.length = fObj["length"] | static_cast<uint8_t>(idx);
                scene.can_frames.push_back(frame);
            }
        }

        if (doc.containsKey("infinitybox_actions")) {
            JsonArray actionsArray = doc["infinitybox_actions"];
            for (JsonObject aObj : actionsArray) {
                SceneInfinityboxAction action;
                action.function_name = aObj["function"] | "";
                action.behavior = aObj["behavior"] | "on";
                action.level = aObj["level"] | 100;
                action.on_ms = aObj["on_ms"] | 500;
                action.off_ms = aObj["off_ms"] | 500;
                action.duration_ms = aObj["duration_ms"] | 0;
                action.release_on_deactivate = aObj["release_on_deactivate"] | true;
                scene.infinitybox_actions.push_back(action);
            }
        }

        if (doc.containsKey("suspension")) {
            JsonObject susp = doc["suspension"];
            scene.suspension.enabled = susp["enabled"] | false;
            scene.suspension.front_left = susp["front_left"] | 0;
            scene.suspension.front_right = susp["front_right"] | 0;
            scene.suspension.rear_left = susp["rear_left"] | 0;
            scene.suspension.rear_right = susp["rear_right"] | 0;
            scene.suspension.calibration_active = susp["calibration_active"] | false;
        }
        
        _engine->addScene(scene);
        saveBehavioralConfig(*_engine); // Auto-save
        
        request->send(200, "application/json", "{\"success\":true,\"id\":\"" + scene.id + "\"}");
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // UTILITIES
    // ═══════════════════════════════════════════════════════════════════════
    
    String extractOutputId(const String& path) {
        // Supported paths:
        //  - /api/outputs/{id}
        //  - /api/outputs/{id}/behavior
        //  - /api/outputs/{id}/deactivate
        //  - /api/output/behavior/{id}
        //  - /api/output/deactivate/{id}
        int start = -1;
        if (path.indexOf("/output/behavior/") >= 0) {
            start = path.indexOf("/output/behavior/") + 17;
        } else if (path.indexOf("/output/deactivate/") >= 0) {
            start = path.indexOf("/output/deactivate/") + 19;
        } else if (path.indexOf("/outputs/") >= 0) {
            start = path.indexOf("/outputs/") + 9;
        }
        if (start < 0) {
            return String();
        }
        int end = path.indexOf("/", start);
        if (end == -1) end = path.length();
        return urlDecode(path.substring(start, end));
    }
    
    String extractSceneId(const String& path) {
        // Extract ID from path like "/api/scenes/activate/scene_123"
        int start = path.lastIndexOf("/") + 1;
        return path.substring(start);
    }

    int hexValue(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    String urlDecode(const String& input) {
        String output;
        output.reserve(input.length());
        for (size_t i = 0; i < input.length(); ++i) {
            char c = input[i];
            if (c == '%' && i + 2 < input.length()) {
                int high = hexValue(input[i + 1]);
                int low = hexValue(input[i + 2]);
                if (high >= 0 && low >= 0) {
                    output += static_cast<char>((high << 4) | low);
                    i += 2;
                    continue;
                }
            }
            if (c == '+') {
                output += ' ';
            } else {
                output += c;
            }
        }
        return output;
    }

    String normalizeKey(const String& input) {
        String output;
        output.reserve(input.length());
        for (size_t i = 0; i < input.length(); ++i) {
            char c = input[i];
            if (isalnum(static_cast<unsigned char>(c))) {
                output += static_cast<char>(tolower(static_cast<unsigned char>(c)));
            }
        }
        return output;
    }

    String resolveOutputId(const String& rawId) {
        if (rawId.isEmpty()) return rawId;
        if (_engine->getOutput(rawId)) return rawId;
        const String normalized = normalizeKey(rawId);
        for (const auto& [key, output] : _engine->getOutputs()) {
            if (normalizeKey(key) == normalized || normalizeKey(output.name) == normalized) {
                return key;
            }
        }
        return rawId;
    }
    
    String behaviorTypeToString(BehaviorType type) {
        switch (type) {
            case BehaviorType::STEADY: return "STEADY";
            case BehaviorType::FLASH: return "FLASH";
            case BehaviorType::PULSE: return "PULSE";
            case BehaviorType::FADE_IN: return "FADE_IN";
            case BehaviorType::FADE_OUT: return "FADE_OUT";
            case BehaviorType::STROBE: return "STROBE";
            case BehaviorType::PATTERN: return "PATTERN";
            case BehaviorType::HOLD_TIMED: return "HOLD_TIMED";
            case BehaviorType::RAMP: return "RAMP";
            case BehaviorType::SCENE_REF: return "SCENE_REF";
            default: return "UNKNOWN";
        }
    }
    
    BehaviorType stringToBehaviorType(const String& str) {
        if (str == "STEADY" || str == "steady") return BehaviorType::STEADY;
        if (str == "FLASH" || str == "flash") return BehaviorType::FLASH;
        if (str == "PULSE" || str == "pulse") return BehaviorType::PULSE;
        if (str == "FADE_IN" || str == "fade_in") return BehaviorType::FADE_IN;
        if (str == "FADE_OUT" || str == "fade_out") return BehaviorType::FADE_OUT;
        if (str == "STROBE" || str == "strobe") return BehaviorType::STROBE;
        if (str == "PATTERN" || str == "pattern") return BehaviorType::PATTERN;
        if (str == "HOLD_TIMED" || str == "hold_timed") return BehaviorType::HOLD_TIMED;
        if (str == "RAMP" || str == "ramp") return BehaviorType::RAMP;
        if (str == "SCENE_REF" || str == "scene_ref") return BehaviorType::SCENE_REF;
        return BehaviorType::STEADY;
    }
};

} // namespace BehavioralOutput
