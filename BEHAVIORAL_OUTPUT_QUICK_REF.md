# 🚀 Behavioral Output System - Quick Reference

## 📋 Table of Contents
1. [Behavior Types Cheat Sheet](#behavior-types-cheat-sheet)
2. [Common Code Snippets](#common-code-snippets)
3. [Web UI Workflow](#web-ui-workflow)
4. [REST API Quick Reference](#rest-api-quick-reference)
5. [Troubleshooting Guide](#troubleshooting-guide)

---

## 🎨 Behavior Types Cheat Sheet

### STEADY - Constant On/Off
```cpp
BehaviorConfig cfg;
cfg.type = BehaviorType::STEADY;
cfg.targetValue = 255;  // 0-255 brightness
```
**Use for**: Headlights, work lights, any constant output

---

### FLASH - Alternating On/Off
```cpp
BehaviorConfig cfg;
cfg.type = BehaviorType::FLASH;
cfg.targetValue = 255;
cfg.period_ms = 500;     // Total cycle time
cfg.dutyCycle = 50;      // % time on (0-100)
cfg.duration_ms = 0;     // 0 = infinite
```
**Use for**: Turn signals, hazard lights, attention-getting

**Examples**:
- Turn signal: `period_ms=500, dutyCycle=50` (250ms on, 250ms off)
- Slow flash: `period_ms=1000, dutyCycle=50`
- Quick blink: `period_ms=300, dutyCycle=60`

---

### PULSE - Smooth Breathing Effect
```cpp
BehaviorConfig cfg;
cfg.type = BehaviorType::PULSE;
cfg.targetValue = 255;
cfg.period_ms = 2000;    // Complete breathe cycle
```
**Use for**: Ambient lighting, status indicators

---

### FADE_IN - One-Time Fade Up
```cpp
BehaviorConfig cfg;
cfg.type = BehaviorType::FADE_IN;
cfg.targetValue = 255;
cfg.fadeTime_ms = 1000;  // Fade duration
cfg.autoOff = false;     // Stay on after fade
cfg.softStart = true;    // Enable POWERCELL soft-start
```
**Use for**: Headlights, cabin lights with smooth turn-on

---

### FADE_OUT - One-Time Fade Down
```cpp
BehaviorConfig cfg;
cfg.type = BehaviorType::FADE_OUT;
cfg.fadeTime_ms = 1000;
cfg.autoOff = true;      // Turn off after fade
```
**Use for**: Theater dimming, smooth power-down

---

### STROBE - Rapid Flash
```cpp
BehaviorConfig cfg;
cfg.type = BehaviorType::STROBE;
cfg.targetValue = 255;
cfg.onTime_ms = 50;      // Flash on duration
cfg.offTime_ms = 50;     // Off duration
```
**Use for**: Emergency lights, alerts

**Examples**:
- Double flash: `onTime=100, offTime=100` (slow)
- Rapid strobe: `onTime=30, offTime=30` (fast)

---

### HOLD_TIMED - Hold Then Auto-Off
```cpp
BehaviorConfig cfg;
cfg.type = BehaviorType::HOLD_TIMED;
cfg.targetValue = 255;
cfg.duration_ms = 5000;  // Hold for 5 seconds
cfg.autoOff = true;
```
**Use for**: Courtesy lights, dome lights, window illumination

---

### PATTERN - Custom Sequence
```cpp
// First define pattern
Pattern myPattern;
myPattern.name = "custom";
myPattern.loop = true;
myPattern.steps.push_back({255, 200, false});  // Full on 200ms
myPattern.steps.push_back({0, 100, false});    // Off 100ms
myPattern.steps.push_back({128, 300, true});   // Half bright 300ms (smooth)
behaviorEngine.addPattern(myPattern);

// Then apply to output
BehaviorConfig cfg;
cfg.type = BehaviorType::PATTERN;
cfg.patternName = "custom";
```
**Use for**: Complex sequences, morse code, custom effects

---

## 💻 Common Code Snippets

### Complete Initialization

```cpp
#include "behavioral_output_integration.h"

void setup() {
    // Initialize system
    initBehavioralOutputSystem(&webServer);
}

void loop() {
    // Update engine and frame synthesis
    updateBehavioralOutputSystem();
}
```

---

### Define an Output

```cpp
OutputChannel myOutput;
myOutput.id = "unique_id";
myOutput.name = "Friendly Name";
myOutput.cellAddress = 1;        // POWERCELL cell 1-254
myOutput.outputNumber = 5;       // Output 1-10
myOutput.description = "Optional description";

behaviorEngine.addOutput(myOutput);
```

---

### Apply Behavior to Output

```cpp
// Method 1: Create config manually
BehaviorConfig behavior;
behavior.type = BehaviorType::FLASH;
behavior.period_ms = 500;
behaviorEngine.setBehavior("output_id", behavior);

// Method 2: Use convenience function (if defined)
activateLeftTurn();  // From integration file
```

---

### Create a Scene

```cpp
Scene myScene;
myScene.id = "scene_id";
myScene.name = "Scene Name";
myScene.exclusive = false;  // Allow other scenes simultaneously

// Add outputs to scene
SceneOutput out1;
out1.outputId = "left_turn";
out1.behavior.type = BehaviorType::FLASH;
out1.behavior.period_ms = 500;

myScene.outputs.push_back(out1);

behaviorEngine.addScene(myScene);
```

---

### Activate/Deactivate Scene

```cpp
// Activate
behaviorEngine.activateScene("scene_id");

// Deactivate
behaviorEngine.deactivateScene("scene_id");

// Check if active
auto* scene = behaviorEngine.getScene("scene_id");
if (scene && scene->isActive) {
    // Scene is running
}
```

---

### Stop Everything (Emergency)

```cpp
// Stop all outputs
const auto& outputs = behaviorEngine.getOutputs();
for (const auto& [id, output] : outputs) {
    behaviorEngine.deactivateOutput(id);
}

// Or deactivate all scenes
const auto& scenes = behaviorEngine.getScenes();
for (const auto& [id, scene] : scenes) {
    behaviorEngine.deactivateScene(id);
}
```

---

### Button Integration Example

```cpp
// In your button handler
void onButton1Pressed() {
    behaviorEngine.activateScene("left_turn");
}

void onButton1Released() {
    behaviorEngine.deactivateScene("left_turn");
}

// Toggle example
bool beaconActive = false;
void onButton2Pressed() {
    if (beaconActive) {
        behaviorEngine.deactivateScene("beacon");
    } else {
        behaviorEngine.activateScene("beacon");
    }
    beaconActive = !beaconActive;
}
```

---

## 🌐 Web UI Workflow

### Access UI
```
http://<device-ip>/behavioral
```

### Step-by-Step Configuration

1. **Navigate to "Outputs" tab**
   - Click "Add Output"
   - Enter name, cell address, output number
   - Save

2. **Navigate to "Behaviors" tab**
   - Select output from dropdown
   - Choose behavior type (Flash, Pulse, etc.)
   - Adjust parameters
   - Click "Apply Behavior"

3. **Navigate to "Scenes" tab**
   - Click "Create Scene"
   - Add outputs to scene
   - Configure each output's behavior
   - Save scene

4. **Navigate to "Live Preview" tab**
   - See real-time output values
   - Verify behaviors are working
   - Click "Stop All" if needed

5. **Navigate to "Simulator" tab**
   - Test predefined scenes
   - Quick buttons for common functions

---

## 🔌 REST API Quick Reference

### Base URL
```
http://<device-ip>/api
```

### Outputs

**List all outputs:**
```bash
GET /api/outputs
```

**Create output:**
```bash
POST /api/outputs
Content-Type: application/json

{
  "id": "my_output",
  "name": "My Output",
  "cellAddress": 1,
  "outputNumber": 5
}
```

**Set behavior:**
```bash
POST /api/outputs/my_output/behavior
Content-Type: application/json

{
  "type": "FLASH",
  "targetValue": 255,
  "period_ms": 500,
  "dutyCycle": 50,
  "duration_ms": 0
}
```

**Stop output:**
```bash
POST /api/outputs/my_output/deactivate
```

**Get current state:**
```bash
GET /api/outputs/state
```

**Stop all outputs:**
```bash
POST /api/outputs/stop-all
```

---

### Scenes

**List scenes:**
```bash
GET /api/scenes
```

**Activate scene:**
```bash
POST /api/scenes/activate/left_turn
```

**Deactivate scene:**
```bash
POST /api/scenes/deactivate/left_turn
```

---

### Example cURL Commands

```bash
# Activate left turn
curl -X POST http://192.168.1.100/api/scenes/activate/left_turn

# Flash an output
curl -X POST http://192.168.1.100/api/outputs/beacon/behavior \
  -H "Content-Type: application/json" \
  -d '{"type":"FLASH","period_ms":500,"dutyCycle":50}'

# Stop all
curl -X POST http://192.168.1.100/api/outputs/stop-all
```

---

## 🔧 Troubleshooting Guide

### Issue: Nothing happens when I activate a behavior

**Checklist:**
- [ ] Is `behaviorEngine.update()` called in `loop()`?
- [ ] Is `powercellSynthesizer->update()` called in `loop()`?
- [ ] Is output defined correctly? (Check cell address, output number)
- [ ] Is CAN bus connected and working?
- [ ] Check serial monitor for error messages

**Debug code:**
```cpp
void loop() {
    behaviorEngine.update();
    powercellSynthesizer->update();
    
    // Debug: Print active outputs
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 1000) {
        const auto& outputs = behaviorEngine.getOutputs();
        for (const auto& [id, output] : outputs) {
            if (output.isActive) {
                Serial.printf("Output %s: %d\n", id.c_str(), output.currentValue);
            }
        }
        lastPrint = millis();
    }
}
```

---

### Issue: Output flashes too fast/slow

**Solution:** Adjust timing parameters

```cpp
// Too fast?
cfg.period_ms = 1000;  // Increase period

// Too slow?
cfg.period_ms = 300;   // Decrease period

// Adjust duty cycle for different on/off ratio
cfg.dutyCycle = 70;  // 70% on, 30% off
```

---

### Issue: Fade not smooth

**Solutions:**

1. **Decrease engine update interval:**
```cpp
behaviorEngine.setUpdateInterval(10);  // 100Hz (very smooth)
```

2. **Decrease transmit interval:**
```cpp
powercellSynthesizer->setTransmitInterval(30);  // ~33Hz
```

3. **Enable soft-start on POWERCELL:**
```cpp
cfg.softStart = true;
```

---

### Issue: Multiple behaviors conflict

**Problem:** Two behaviors targeting same output

**Solution 1 - Use priority:**
```cpp
// Higher priority wins
BehaviorConfig emergency;
emergency.priority = 200;

BehaviorConfig normal;
normal.priority = 100;
```

**Solution 2 - Use exclusive scenes:**
```cpp
Scene emergencyScene;
emergencyScene.exclusive = true;  // Deactivates other scenes
```

---

### Issue: Outputs turn off unexpectedly

**Likely cause:** Another system is sending CAN frames

**Solution:** Ensure only one system controls POWERCELL outputs. The behavioral system should be the sole source of output frames.

**Debug:** Monitor CAN bus to see all traffic:
```cpp
CanManager.setDebugMode(true);
```

---

### Issue: Web UI not loading

**Checklist:**
- [ ] Is web server initialized?
- [ ] Did you register API endpoints?
- [ ] Is device on network?
- [ ] Try accessing `/behavioral` directly

**Debug:**
```cpp
// Add to setup()
webServer.on("/test", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Web server works!");
});
```

---

## 📊 Performance Tips

### Optimize for Smoothness
```cpp
behaviorEngine.setUpdateInterval(20);  // 50Hz
powercellSynthesizer->setTransmitInterval(50);  // 20Hz
```

### Optimize for Low CPU Usage
```cpp
behaviorEngine.setUpdateInterval(50);  // 20Hz
powercellSynthesizer->setTransmitInterval(100);  // 10Hz
```

### Optimize for Low CAN Traffic
```cpp
powercellSynthesizer->setTransmitInterval(200);  // 5Hz
// Note: Some fast behaviors may look choppy
```

---

## 🎯 Best Practices

1. **Always define outputs before using them**
   ```cpp
   behaviorEngine.addOutput(output);  // First
   behaviorEngine.setBehavior(id, cfg);  // Then
   ```

2. **Use scenes for coordinated actions**
   - Better than managing individual outputs
   - Cleaner code
   - Easy to activate/deactivate

3. **Set reasonable durations**
   ```cpp
   cfg.duration_ms = 5000;  // Auto-off after 5s
   cfg.autoOff = true;
   ```

4. **Use descriptive IDs and names**
   ```cpp
   output.id = "left_turn";  // Good
   output.id = "out1";       // Bad
   ```

5. **Test with simulator before hardware**
   - Use web UI simulator
   - Verify timing
   - Check for conflicts

---

## 📦 Pre-configured Scenes Available

(From `behavioral_output_integration.h`)

| Scene ID | Description | Outputs |
|----------|-------------|---------|
| `left_turn` | Flash left turn signal | left_turn |
| `right_turn` | Flash right turn signal | right_turn |
| `four_way` | Flash both turn signals | left_turn, right_turn |
| `beacon` | Emergency beacon strobe | beacon |
| `headlights_on` | Fade headlights on | headlights |
| `brake_pulse` | Pulse brake lights (3s) | brake_lights |

**Usage:**
```cpp
behaviorEngine.activateScene("four_way");
```

---

## 🚀 Quick Start Checklist

- [ ] Include framework headers
- [ ] Call `initBehavioralOutputSystem()` in `setup()`
- [ ] Call `updateBehavioralOutputSystem()` in `loop()`
- [ ] Define your outputs
- [ ] Create scenes for common functions
- [ ] Map scenes to buttons/inputs
- [ ] Test with web UI
- [ ] Deploy and enjoy! ✅

---

**Need more help?** Check `BEHAVIORAL_OUTPUT_FRAMEWORK.md` for detailed documentation.

**Version**: 1.0.0  
**Updated**: 2026-02-03
