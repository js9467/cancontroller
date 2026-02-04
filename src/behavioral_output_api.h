#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "output_behavior_engine.h"
#include "output_frame_synthesizer.h"
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
        
        // GET /api/outputs - List all outputs
        _server->on("/api/outputs", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String json = serializeOutputs();
            request->send(200, "application/json", json);
        });
        
        // POST /api/outputs - Create new output
        _server->on("/api/outputs", HTTP_POST, [](AsyncWebServerRequest* request){},
            nullptr,
            [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
                handleCreateOutput(request, data, len);
            });
        
        // GET /api/outputs/{id} - Get specific output
        _server->on("/api/outputs/*", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String path = request->url();
            String id = extractOutputId(path);
            
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
            String id = extractOutputId(path);
            
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
        
        const auto& outputs = _engine->getOutputs();
        for (const auto& [id, output] : outputs) {
            JsonObject obj = array.createNestedObject();
            obj["id"] = output.id;
            obj["name"] = output.name;
            obj["currentValue"] = output.currentState ? 255 : 0;
            obj["isActive"] = output.isActive;
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
        }
        
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
        String id = extractOutputId(path);
        
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
        DynamicJsonDocument doc(4096);
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
                so.behavior.type = stringToBehaviorType(outObj["behavior_type"] | "STEADY");
                so.behavior.targetValue = outObj["target_value"] | 255;
                so.behavior.period_ms = outObj["period_ms"] | 1000;
                so.behavior.dutyCycle = outObj["duty_cycle"] | 50;
                so.behavior.fadeTime_ms = outObj["fade_time_ms"] | 500;
                so.behavior.onTime_ms = outObj["on_time_ms"] | 500;
                so.behavior.offTime_ms = outObj["off_time_ms"] | 500;
                so.behavior.softStart = outObj["soft_start"] | false;
                scene.outputs.push_back(so);
            }
        }
        
        _engine->addScene(scene);
        saveBehavioralConfig(*_engine); // Auto-save
        
        request->send(200, "application/json", "{\"success\":true,\"id\":\"" + scene.id + "\"}");
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // UTILITIES
    // ═══════════════════════════════════════════════════════════════════════
    
    String extractOutputId(const String& path) {
        // Extract ID from path like "/api/outputs/out_123/behavior"
        int start = path.indexOf("/outputs/") + 9;
        int end = path.indexOf("/", start);
        if (end == -1) end = path.length();
        return path.substring(start, end);
    }
    
    String extractSceneId(const String& path) {
        // Extract ID from path like "/api/scenes/activate/scene_123"
        int start = path.lastIndexOf("/") + 1;
        return path.substring(start);
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
        if (str == "STEADY") return BehaviorType::STEADY;
        if (str == "FLASH") return BehaviorType::FLASH;
        if (str == "PULSE") return BehaviorType::PULSE;
        if (str == "FADE_IN") return BehaviorType::FADE_IN;
        if (str == "FADE_OUT") return BehaviorType::FADE_OUT;
        if (str == "STROBE") return BehaviorType::STROBE;
        if (str == "PATTERN") return BehaviorType::PATTERN;
        if (str == "HOLD_TIMED") return BehaviorType::HOLD_TIMED;
        if (str == "RAMP") return BehaviorType::RAMP;
        if (str == "SCENE_REF") return BehaviorType::SCENE_REF;
        return BehaviorType::STEADY;
    }
};

} // namespace BehavioralOutput
