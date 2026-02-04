# 🎛️ Behavioral Output Control Framework

## Overview

A complete intent-based output control system that solves the fundamental limitation of POWERCELL NGX: **it only accepts static state frames, not behavioral commands**.

This framework provides an abstraction layer that allows you to define **what you want outputs to do** (flash, fade, pulse, hold) and handles the continuous frame synthesis required to make it happen.

---

## 🎯 Core Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         USER INTERFACE                          │
│  (Define outputs, behaviors, patterns, scenes)                  │
└─────────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     BEHAVIOR ENGINE                             │
│  • Manages output definitions                                   │
│  • Executes timing logic for each behavior                      │
│  • Handles scenes and priority resolution                       │
│  • Updates at 50Hz (20ms intervals)                             │
└─────────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   FRAME SYNTHESIZER                             │
│  • Collects current state from all outputs                      │
│  • Groups outputs by cell address                               │
│  • Generates complete 8-byte POWERCELL state frames             │
│  • Transmits at 20Hz (50ms intervals)                           │
└─────────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     CAN NETWORK                                 │
│  • POWERCELL NGX receives complete state frames                 │
│  • Other devices can use custom frame mappings                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📦 Components

### 1. **Output Behavior Engine** (`output_behavior_engine.h`)

**Core engine that owns all state and timing logic.**

#### Key Features:
- **Output Definitions**: Physical mapping (cell address, output number)
- **Behavior Types**: STEADY, FLASH, PULSE, FADE_IN, FADE_OUT, STROBE, PATTERN, HOLD_TIMED, RAMP
- **Scene Management**: Coordinate multiple outputs
- **Pattern System**: Custom timing sequences
- **Priority Resolution**: Handle conflicts gracefully

#### Behavior Types Explained:

| Behavior | Description | Parameters |
|----------|-------------|------------|
| **STEADY** | Constant on/off state | `targetValue` |
| **FLASH** | Alternating on/off | `period_ms`, `dutyCycle` (%) |
| **PULSE** | Smooth sine wave (breathing) | `period_ms` |
| **FADE_IN** | One-time fade from 0→target | `fadeTime_ms` |
| **FADE_OUT** | One-time fade from current→0 | `fadeTime_ms` |
| **STROBE** | Rapid flashing | `onTime_ms`, `offTime_ms` |
| **PATTERN** | Custom sequence | `patternName` |
| **HOLD_TIMED** | Steady for fixed duration | `duration_ms` |
| **RAMP** | Linear transition over time | `fadeTime_ms` |

---

### 2. **Frame Synthesizer** (`output_frame_synthesizer.h`)

**Merges behavioral states into complete POWERCELL CAN frames.**

#### Responsibilities:
- Collect current values from all active outputs
- Group outputs by cell address
- Generate complete 8-byte state frames (all outputs 1-8)
- Transmit frames at configurable rate (default 50ms)
- Handle soft-start configuration frames

#### Why Complete Frames Matter:
POWERCELL NGX interprets each frame as **authoritative complete state**. Sending partial updates (e.g., only output 1) will turn OFF all other outputs. The synthesizer ensures every frame represents the full desired state.

---

### 3. **Web UI** (`behavioral_output_ui.h`)

**Intuitive interface for configuration and testing.**

#### Features:
- **Output Manager**: Define outputs with names and mappings
- **Behavior Designer**: Configure flash patterns, fade curves, etc.
- **Pattern Library**: Create custom timing sequences
- **Scene Builder**: Coordinate multiple outputs
- **Live Preview**: Real-time visualization
- **Simulator**: Test without hardware

---

### 4. **REST API** (`behavioral_output_api.h`)

**HTTP endpoints for runtime control.**

#### Key Endpoints:

```
GET    /api/outputs              List all outputs
POST   /api/outputs              Create output
GET    /api/outputs/{id}         Get output details
DELETE /api/outputs/{id}         Remove output

POST   /api/outputs/{id}/behavior    Set behavior
POST   /api/outputs/{id}/deactivate  Stop output
GET    /api/outputs/state            Get current values
POST   /api/outputs/stop-all         Emergency stop

GET    /api/patterns             List patterns
POST   /api/patterns             Create pattern

GET    /api/scenes               List scenes
POST   /api/scenes               Create scene
POST   /api/scenes/activate/{id} Activate scene
POST   /api/scenes/deactivate/{id} Deactivate scene
```

---

## 🚀 Quick Start

### 1. **Include Headers**

```cpp
#include "output_behavior_engine.h"
#include "output_frame_synthesizer.h"
#include "behavioral_output_api.h"

using namespace BehavioralOutput;
```

### 2. **Create Global Instances**

```cpp
BehaviorEngine behaviorEngine;
PowercellSynthesizer* powercellSynthesizer = nullptr;
```

### 3. **Initialize in setup()**

```cpp
void setup() {
    // Create synthesizer with CAN send callback
    powercellSynthesizer = new PowercellSynthesizer(
        &behaviorEngine,
        [](uint32_t pgn, uint8_t* data) {
            uint32_t canId = (pgn << 8); // Convert PGN to CAN ID
            CanManager.sendFrame(canId, data, 8);
        }
    );
    
    // Configure update rates
    behaviorEngine.setUpdateInterval(20);  // 50Hz
    powercellSynthesizer->setTransmitInterval(50);  // 20Hz
}
```

### 4. **Update in loop()**

```cpp
void loop() {
    behaviorEngine.update();
    powercellSynthesizer->update();
}
```

---

## 💡 Usage Examples

### Example 1: Simple Flashing Turn Signal

```cpp
// Define the output
OutputChannel leftTurn;
leftTurn.id = "left_turn";
leftTurn.name = "Left Turn Signal";
leftTurn.cellAddress = 1;
leftTurn.outputNumber = 1;
behaviorEngine.addOutput(leftTurn);

// Configure flash behavior
BehaviorConfig flashBehavior;
flashBehavior.type = BehaviorType::FLASH;
flashBehavior.targetValue = 255;
flashBehavior.period_ms = 500;      // Flash every 500ms
flashBehavior.dutyCycle = 50;       // 50% on, 50% off
flashBehavior.duration_ms = 0;      // Infinite (until deactivated)

// Apply behavior
behaviorEngine.setBehavior("left_turn", flashBehavior);

// Later: Stop flashing
behaviorEngine.deactivateOutput("left_turn");
```

### Example 2: Window Light Hold for 5 Seconds

```cpp
OutputChannel window;
window.id = "window_light";
window.name = "Window Illumination";
window.cellAddress = 1;
window.outputNumber = 7;
behaviorEngine.addOutput(window);

// Hold at full brightness for 5 seconds, then auto-off
BehaviorConfig holdBehavior;
holdBehavior.type = BehaviorType::HOLD_TIMED;
holdBehavior.targetValue = 255;
holdBehavior.duration_ms = 5000;
holdBehavior.autoOff = true;

behaviorEngine.setBehavior("window_light", holdBehavior);
// Output will automatically turn off after 5 seconds
```

### Example 3: Beacon with Custom Flash Pattern

```cpp
// Define custom pattern (triple flash)
Pattern beaconPattern;
beaconPattern.name = "triple_flash";
beaconPattern.loop = true;

beaconPattern.steps.push_back({255, 100, false});  // On 100ms
beaconPattern.steps.push_back({0, 100, false});    // Off 100ms
beaconPattern.steps.push_back({255, 100, false});  // On 100ms
beaconPattern.steps.push_back({0, 100, false});    // Off 100ms
beaconPattern.steps.push_back({255, 100, false});  // On 100ms
beaconPattern.steps.push_back({0, 800, false});    // Off 800ms (long pause)

behaviorEngine.addPattern(beaconPattern);

// Apply pattern to output
BehaviorConfig beaconBehavior;
beaconBehavior.type = BehaviorType::PATTERN;
beaconBehavior.patternName = "triple_flash";

behaviorEngine.setBehavior("beacon", beaconBehavior);
```

### Example 4: Scene with Multiple Outputs

```cpp
// Create 4-way flasher scene
Scene fourWay;
fourWay.id = "four_way";
fourWay.name = "4-Way Hazard Flashers";
fourWay.exclusive = false;

// Left turn output
SceneOutput leftOut;
leftOut.outputId = "left_turn";
leftOut.behavior.type = BehaviorType::FLASH;
leftOut.behavior.targetValue = 255;
leftOut.behavior.period_ms = 500;
leftOut.behavior.dutyCycle = 50;

// Right turn output (same behavior)
SceneOutput rightOut;
rightOut.outputId = "right_turn";
rightOut.behavior = leftOut.behavior;

fourWay.outputs.push_back(leftOut);
fourWay.outputs.push_back(rightOut);

behaviorEngine.addScene(fourWay);

// Activate the scene
behaviorEngine.activateScene("four_way");

// Later: Deactivate
behaviorEngine.deactivateScene("four_way");
```

### Example 5: Headlights with Soft Fade-In

```cpp
OutputChannel headlights;
headlights.id = "headlights";
headlights.name = "Main Headlights";
headlights.cellAddress = 1;
headlights.outputNumber = 5;
behaviorEngine.addOutput(headlights);

// Fade in over 1 second, then stay on
BehaviorConfig fadeIn;
fadeIn.type = BehaviorType::FADE_IN;
fadeIn.targetValue = 255;
fadeIn.fadeTime_ms = 1000;
fadeIn.softStart = true;      // Enable POWERCELL soft-start
fadeIn.autoOff = false;       // Don't turn off after fade completes

behaviorEngine.setBehavior("headlights", fadeIn);
```

---

## 🔧 Advanced Features

### Priority System

When multiple behaviors target the same output, **priority** determines which wins:

```cpp
// High priority behavior (emergency)
BehaviorConfig emergency;
emergency.type = BehaviorType::STROBE;
emergency.priority = 200;  // Higher than default 100

// Low priority behavior (ambient)
BehaviorConfig ambient;
ambient.type = BehaviorType::PULSE;
ambient.priority = 50;

// Emergency will override ambient
```

### Non-POWERCELL Devices

For devices that don't use POWERCELL protocol, use **GenericCanController**:

```cpp
GenericCanController genericCan(
    &behaviorEngine,
    [](uint32_t canId, uint8_t* data, uint8_t len) {
        CanManager.sendFrame(canId, data, len);
    }
);

// Map output to custom CAN frame
genericCan.mapOutput("custom_device", 
    [](uint8_t value, uint32_t& canId, uint8_t* data, uint8_t& len) {
        canId = 0x123;
        data[0] = value;
        data[1] = 0x00;
        len = 2;
    }
);
```

### Performance Tuning

```cpp
// Adjust update rates for performance vs. responsiveness
behaviorEngine.setUpdateInterval(20);  // 50Hz (default: smooth)
behaviorEngine.setUpdateInterval(50);  // 20Hz (lighter CPU load)

powercellSynthesizer->setTransmitInterval(50);  // 20Hz (default)
powercellSynthesizer->setTransmitInterval(100); // 10Hz (reduce CAN traffic)
```

---

## 🎨 Web Interface

Access the UI at: **`http://<device-ip>/behavioral`**

### Workflow:
1. **Define Outputs** → Map to physical hardware
2. **Create Patterns** → Build custom sequences (optional)
3. **Configure Behaviors** → Assign flash/fade/pulse to outputs
4. **Build Scenes** → Coordinate multiple outputs
5. **Test** → Use live preview and simulator
6. **Activate** → Button press or API call

---

## 📊 Comparison: Old vs. New

| Feature | Old InfinityBox Config | New Behavioral Framework |
|---------|------------------------|--------------------------|
| Flash | ❌ Single frame = no flash | ✅ Continuous synthesis |
| Fade | ❌ Static soft-start only | ✅ True fade-in/out |
| Scenes | ❌ No coordination | ✅ Full scene support |
| Custom Patterns | ❌ Not possible | ✅ Unlimited patterns |
| Timed Behaviors | ❌ Manual timing | ✅ Built-in duration |
| Mixed Inputs | ❌ Conflicts overwrite | ✅ Priority resolution |
| Flexibility | ⚠️ Template-based | ✅ Full customization |

---

## 🛡️ Architecture Benefits

### 1. **Separation of Concerns**
- **UI**: Expresses user intent
- **Engine**: Handles timing and logic
- **Synthesizer**: Manages hardware protocol
- **CAN**: Just transport layer

### 2. **Extensibility**
- Add new behavior types easily
- Support any CAN device via custom mappers
- Mix POWERCELL and non-POWERCELL devices

### 3. **Testability**
- Simulator mode (no CAN transmission)
- Live preview without hardware
- Pattern testing before deployment

### 4. **Maintainability**
- Clear data structures
- Well-documented code
- REST API for external integration

---

## 🔍 Technical Details

### Frame Transmission Strategy

**Problem**: POWERCELL interprets each frame as complete state. Sending only Output 1 turns off Outputs 2-8.

**Solution**: Frame synthesizer maintains **complete cell state** and always sends all 8 outputs:

```cpp
// Frame structure (8 bytes):
// [Out1, Out2, Out3, Out4, Out5, Out6, Out7, Out8]

// Even if only Output 1 is active:
uint8_t data[8] = {255, 0, 0, 0, 0, 0, 0, 0};

// Not just: {255} ← This would be interpreted incorrectly
```

### Timing Engine

Behaviors are evaluated every 20ms (50Hz) by default:

```cpp
unsigned long elapsed = millis() - behavior.activatedAt;

switch (behavior.type) {
    case FLASH:
        unsigned long cyclePos = elapsed % period_ms;
        return (cyclePos < onTime_ms) ? targetValue : 0;
    
    case PULSE:
        float phase = (float)(elapsed % period_ms) / period_ms * 2.0 * PI;
        return (uint8_t)(targetValue * (sin(phase) + 1.0) / 2.0);
    
    // ... other behaviors
}
```

### Scene Activation

Scenes apply behaviors to multiple outputs atomically:

```cpp
bool activateScene(sceneId) {
    Scene& scene = scenes[sceneId];
    
    if (scene.exclusive) {
        deactivateAllOtherScenes();
    }
    
    for (auto& sceneOutput : scene.outputs) {
        setBehavior(sceneOutput.outputId, sceneOutput.behavior);
    }
    
    scene.isActive = true;
}
```

---

## 📝 Integration Checklist

- [ ] Include framework headers
- [ ] Create global engine and synthesizer instances
- [ ] Initialize in `setup()` with CAN send callback
- [ ] Define outputs with physical mappings
- [ ] Create predefined scenes (turn signals, etc.)
- [ ] Call `update()` in main loop
- [ ] Register REST API endpoints (if using web server)
- [ ] Map button inputs to scene activation
- [ ] Test with simulator before hardware deployment

---

## 🎯 Common Patterns

### Pattern 1: Button-Activated Scene

```cpp
void onLeftTurnButtonPressed() {
    behaviorEngine.activateScene("left_turn");
}

void onLeftTurnButtonReleased() {
    behaviorEngine.deactivateScene("left_turn");
}
```

### Pattern 2: Timed Alert

```cpp
void alertSequence() {
    // Flash beacon 3 times then stop
    BehaviorConfig alert;
    alert.type = BehaviorType::FLASH;
    alert.period_ms = 300;
    alert.duration_ms = 1800;  // 3 flashes × 600ms
    alert.autoOff = true;
    
    behaviorEngine.setBehavior("beacon", alert);
}
```

### Pattern 3: Conditional Behavior

```cpp
void updateBrakeLights(bool braking, bool abs_active) {
    if (abs_active) {
        // Rapid flash during ABS activation
        BehaviorConfig absPulse;
        absPulse.type = BehaviorType::FLASH;
        absPulse.period_ms = 200;
        behaviorEngine.setBehavior("brake_lights", absPulse);
    } else if (braking) {
        // Steady on
        BehaviorConfig steadyBrake;
        steadyBrake.type = BehaviorType::STEADY;
        steadyBrake.targetValue = 255;
        behaviorEngine.setBehavior("brake_lights", steadyBrake);
    } else {
        // Off
        behaviorEngine.deactivateOutput("brake_lights");
    }
}
```

---

## 🚨 Troubleshooting

### Issue: Outputs not responding
- **Check**: Is `behaviorEngine.update()` being called in loop?
- **Check**: Is `powercellSynthesizer->update()` being called?
- **Check**: Are CAN frames actually transmitting? (use CAN monitor)

### Issue: Flashing too fast/slow
- **Solution**: Adjust `period_ms` in behavior configuration
- **Note**: Minimum reliable period is ~100ms due to CAN bus timing

### Issue: Outputs conflict/flicker
- **Check**: Are multiple behaviors targeting same output?
- **Solution**: Use priority system or exclusive scenes

### Issue: Fade not smooth
- **Solution**: Decrease `behaviorEngine.setUpdateInterval()` (20ms recommended)
- **Solution**: Ensure transmit rate is fast enough (50ms default)

---

## 📚 File Reference

| File | Purpose |
|------|---------|
| `output_behavior_engine.h` | Core engine with timing logic |
| `output_frame_synthesizer.h` | POWERCELL frame generation |
| `behavioral_output_ui.h` | Web interface HTML/JS |
| `behavioral_output_api.h` | REST API endpoints |
| `behavioral_output_integration.h` | Example integration code |

---

## 🎓 Next Steps

1. Review the example integration file
2. Define your physical outputs
3. Create scenes for common use cases
4. Test with the web UI
5. Map to button inputs
6. Enjoy flexible, powerful output control! 🚀

---

**Framework Version**: 1.0.0  
**Author**: Behavioral Output Control System  
**License**: Part of Bronco Controls Project
