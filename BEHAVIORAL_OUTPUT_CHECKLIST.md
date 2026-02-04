# ✅ Behavioral Output Framework - Integration Checklist

## Pre-Integration Verification

- [ ] Review [BEHAVIORAL_OUTPUT_FRAMEWORK.md](BEHAVIORAL_OUTPUT_FRAMEWORK.md) for architecture overview
- [ ] Review [BEHAVIORAL_OUTPUT_QUICK_REF.md](BEHAVIORAL_OUTPUT_QUICK_REF.md) for usage examples
- [ ] Review [BEHAVIORAL_OUTPUT_SUMMARY.md](BEHAVIORAL_OUTPUT_SUMMARY.md) for implementation details

---

## Required Files

Verify these files are present in your `src/` directory:

- [ ] `output_behavior_engine.h` - Core behavior engine
- [ ] `output_frame_synthesizer.h` - POWERCELL frame synthesis
- [ ] `behavioral_output_ui.h` - Web interface
- [ ] `behavioral_output_api.h` - REST API endpoints
- [ ] `behavioral_output_integration.h` - Integration example

---

## Code Integration Steps

### Step 1: Add Includes to main.cpp

```cpp
#include "behavioral_output_integration.h"
```

### Step 2: Declare Global Variables (if not using integration file)

```cpp
// Add to global scope
BehaviorEngine behaviorEngine;
PowercellSynthesizer* powercellSynthesizer = nullptr;
BehavioralOutputAPI* outputAPI = nullptr;
```

### Step 3: Initialize in setup()

```cpp
void setup() {
    // ... existing setup code ...
    
    // Initialize web server first (if not already done)
    // webServer = new AsyncWebServer(80);
    
    // Initialize behavioral output system
    initBehavioralOutputSystem(&webServer);
    
    // ... rest of setup ...
}
```

### Step 4: Update in loop()

```cpp
void loop() {
    // ... existing loop code ...
    
    // Update behavioral output system
    updateBehavioralOutputSystem();
    
    // ... rest of loop ...
}
```

### Step 5: Map Button Inputs (Optional)

```cpp
// Example: In your button handler
void handleButtonPress(uint8_t buttonId, bool pressed) {
    if (pressed) {
        onButtonPressed(buttonId);  // From integration file
    } else {
        onButtonReleased(buttonId);  // From integration file
    }
}
```

---

## Configuration Steps

### Step 1: Define Your Outputs

Edit `setupPredefinedOutputs()` in `behavioral_output_integration.h` or use web UI:

```cpp
OutputChannel myOutput;
myOutput.id = "unique_id";
myOutput.name = "Display Name";
myOutput.cellAddress = 1;     // POWERCELL cell address
myOutput.outputNumber = 5;    // Output 1-10
behaviorEngine.addOutput(myOutput);
```

### Step 2: Create Common Scenes

Edit `setupPredefinedScenes()` in `behavioral_output_integration.h`:

```cpp
Scene myScene;
myScene.id = "scene_id";
myScene.name = "Scene Name";

SceneOutput out;
out.outputId = "myOutput";
out.behavior.type = BehaviorType::FLASH;
out.behavior.period_ms = 500;

myScene.outputs.push_back(out);
behaviorEngine.addScene(myScene);
```

### Step 3: Map to Physical Buttons

Create convenience functions or call directly:

```cpp
void onLeftTurnButtonPressed() {
    behaviorEngine.activateScene("left_turn");
}

void onLeftTurnButtonReleased() {
    behaviorEngine.deactivateScene("left_turn");
}
```

---

## Testing Checklist

### Phase 1: Compile & Upload

- [ ] Project compiles without errors
- [ ] Upload to ESP32 successful
- [ ] Serial monitor shows initialization messages
- [ ] No crashes or reboots

**Expected Serial Output:**
```
[Behavioral Output] Initializing system...
[Behavioral Output] System initialized
```

### Phase 2: Web UI Access

- [ ] Connect to device's IP address
- [ ] Navigate to `http://<device-ip>/behavioral`
- [ ] UI loads correctly
- [ ] Navigation works (tabs switch properly)

### Phase 3: Output Configuration (Web UI)

- [ ] Navigate to "Outputs" tab
- [ ] Click "Add Output"
- [ ] Create test output (name, cell address, output number)
- [ ] Verify output appears in list
- [ ] Click "Edit" - verify data loads correctly

### Phase 4: Behavior Testing (Web UI)

- [ ] Navigate to "Behaviors" tab
- [ ] Select an output from dropdown
- [ ] Choose "Flash" behavior type
- [ ] Set parameters (period: 500ms, duty: 50%)
- [ ] Click "Apply Behavior"
- [ ] **Hardware Check**: Verify output physically flashes

### Phase 5: Scene Testing (Web UI)

- [ ] Navigate to "Scenes" tab  
- [ ] Verify predefined scenes are listed
- [ ] Click "Activate" on a scene
- [ ] **Hardware Check**: Verify scene executes correctly
- [ ] Click "Deactivate"
- [ ] **Hardware Check**: Verify scene stops

### Phase 6: Live Preview

- [ ] Navigate to "Preview" tab
- [ ] Activate a behavior or scene
- [ ] Verify preview shows real-time values
- [ ] Verify indicators pulse/glow correctly
- [ ] Click "Stop All" - verify everything stops

### Phase 7: API Testing (Optional)

Use curl or Postman to test endpoints:

```bash
# List outputs
curl http://<device-ip>/api/outputs

# Activate scene
curl -X POST http://<device-ip>/api/scenes/activate/left_turn

# Stop all
curl -X POST http://<device-ip>/api/outputs/stop-all
```

- [ ] GET `/api/outputs` returns output list
- [ ] POST `/api/outputs` creates new output
- [ ] POST `/api/scenes/activate/{id}` activates scene
- [ ] POST `/api/outputs/stop-all` stops everything

---

## Hardware Integration Checklist

### CAN Bus Verification

- [ ] CAN transceiver connected correctly
- [ ] CAN_TX and CAN_RX pins configured
- [ ] CAN bus termination resistors installed
- [ ] POWERCELL NGX connected to CAN bus
- [ ] Power to POWERCELL verified

### POWERCELL Configuration

- [ ] POWERCELL cell addresses match your output definitions
- [ ] Outputs wired to physical loads (LEDs, relays, etc.)
- [ ] Ground reference shared between ESP32 and POWERCELL
- [ ] Test each output individually with web UI

### CAN Frame Monitoring (Debugging)

If you have a CAN analyzer:

- [ ] Verify frames are being transmitted
- [ ] Check frame PGN values (should be 0xFF50 + cell_address)
- [ ] Verify 8-byte data payload
- [ ] Check transmission rate (~20Hz)

**Expected Frame Example:**
```
PGN: 0xFF51 (Cell 1, Output State)
Data: [255, 0, 0, 0, 0, 0, 0, 0]  // Output 1 on, others off
```

---

## Performance Verification

### CPU Usage

- [ ] Monitor CPU usage in loop (should be <5% for behavioral system)
- [ ] No noticeable lag in UI responsiveness
- [ ] No watchdog timer resets

### Memory Usage

- [ ] Check heap usage after initialization
- [ ] Verify no memory leaks (heap stable over time)
- [ ] Recommended: >100KB free heap

### Timing Accuracy

- [ ] Flash behaviors have correct period (use stopwatch)
- [ ] Fade behaviors are smooth (no stepping)
- [ ] Pulse behaviors are rhythmic (not choppy)

**Test Flash Accuracy:**
1. Set period to 1000ms, duty 50%
2. Count flashes over 60 seconds
3. Should be ~60 flashes (±2)

---

## Common Issues & Solutions

### Issue: Outputs don't respond

**Checklist:**
- [ ] Is `updateBehavioralOutputSystem()` called in loop?
- [ ] Are CAN frames being transmitted? (check with analyzer)
- [ ] Is POWERCELL receiving power?
- [ ] Are cell addresses correct?
- [ ] Try direct CAN test (bypass framework)

### Issue: Flashing is choppy

**Solutions:**
- [ ] Decrease `behaviorEngine.setUpdateInterval()` to 10-20ms
- [ ] Decrease `powercellSynthesizer->setTransmitInterval()` to 30-50ms
- [ ] Verify CAN bus is not overloaded

### Issue: Web UI doesn't load

**Checklist:**
- [ ] Is web server initialized and started?
- [ ] Did you call `outputAPI->registerEndpoints()`?
- [ ] Is device on network and reachable?
- [ ] Try accessing `/test` endpoint
- [ ] Check browser console for JavaScript errors

### Issue: Multiple behaviors conflict

**Solutions:**
- [ ] Use priority system (higher priority wins)
- [ ] Use exclusive scenes (deactivate others)
- [ ] Review scene activation logic

---

## Final Validation

### Functional Tests

- [ ] **Test 1**: Activate left turn - flashes correctly
- [ ] **Test 2**: Activate right turn - flashes correctly
- [ ] **Test 3**: Activate 4-way - both flash simultaneously
- [ ] **Test 4**: Activate beacon - strobes as expected
- [ ] **Test 5**: Fade headlights on - smooth transition
- [ ] **Test 6**: Timed hold (5s) - auto-offs correctly
- [ ] **Test 7**: Stop all - everything turns off immediately

### Integration Tests

- [ ] **Test 8**: Button press activates scene
- [ ] **Test 9**: Button release deactivates scene
- [ ] **Test 10**: Multiple scenes can run simultaneously (if not exclusive)
- [ ] **Test 11**: API call activates scene
- [ ] **Test 12**: Web UI and buttons work together

### Stress Tests

- [ ] **Test 13**: Activate all outputs simultaneously - no crashes
- [ ] **Test 14**: Rapidly toggle scenes - no corruption
- [ ] **Test 15**: Run for 1 hour - no memory leaks or watchdog resets

---

## Documentation

- [ ] Document your output definitions (names, mappings)
- [ ] Document custom scenes created
- [ ] Document button mappings
- [ ] Update project README with behavioral system info
- [ ] Create user guide if deploying to others

---

## Deployment Checklist

- [ ] All tests pass
- [ ] Performance acceptable
- [ ] No known bugs
- [ ] Code reviewed and cleaned
- [ ] Version number updated
- [ ] Backup created
- [ ] Ready for production! 🚀

---

## Post-Deployment

### Monitoring

- [ ] Monitor for crashes or unexpected behavior
- [ ] Check CAN bus load (should be reasonable)
- [ ] Gather user feedback
- [ ] Document any issues

### Maintenance

- [ ] Keep documentation updated
- [ ] Add new scenes as requested
- [ ] Optimize performance if needed
- [ ] Plan feature additions

---

## Support Resources

- **Documentation**: 
  - [BEHAVIORAL_OUTPUT_FRAMEWORK.md](BEHAVIORAL_OUTPUT_FRAMEWORK.md)
  - [BEHAVIORAL_OUTPUT_QUICK_REF.md](BEHAVIORAL_OUTPUT_QUICK_REF.md)
  - [BEHAVIORAL_OUTPUT_SUMMARY.md](BEHAVIORAL_OUTPUT_SUMMARY.md)

- **Example Code**: 
  - [behavioral_output_integration.h](src/behavioral_output_integration.h)

- **Web UI**: 
  - Access at `http://<device-ip>/behavioral`

---

## ✅ Sign-Off

Once all items are checked:

**Integrated by**: _______________  
**Date**: _______________  
**Version**: _______________  
**Status**: ✅ Production Ready

---

**Good luck with your integration! 🎉**

If you encounter any issues not covered in this checklist, review the comprehensive documentation or examine the example integration code.
