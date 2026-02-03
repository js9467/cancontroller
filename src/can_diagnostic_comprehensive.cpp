/**
 * @file can_diagnostic_comprehensive.cpp
 * @brief Comprehensive CAN Bus Diagnostic Tool for Waveshare ESP32-S3-Touch-LCD-7
 * 
 * This diagnostic tool performs a complete hardware verification and CAN bus test.
 * 
 * USAGE:
 *   1. Build with: pio run -e can_test_minimal -t upload
 *   2. Monitor with: pio device monitor -e can_test_minimal
 *   3. Follow the on-screen test results
 * 
 * WHAT IT TESTS:
 *   - I2C bus functionality and CH422G detection
 *   - GPIO pin state verification (RX/TX pins)
 *   - CH422G gate register configuration
 *   - TWAI driver initialization
 *   - CAN bus frame reception (live traffic test)
 *   - Multiple gate enable strategies
 */

#include <Arduino.h>
#include <Wire.h>
#include <driver/twai.h>
#include <driver/gpio.h>

// Hardware constants from hardware_config.h
constexpr uint8_t CAN_TX_PIN = 20;
constexpr uint8_t CAN_RX_PIN = 19;
constexpr uint8_t I2C_SDA_PIN = 8;
constexpr uint8_t I2C_SCL_PIN = 9;
constexpr uint8_t CH422G_ADDR_CMD = 0x24;
constexpr uint8_t CH422G_ADDR_OUT = 0x38;
constexpr uint8_t USB_SEL_BIT = 5;

// Gate enable values from known-working configurations
constexpr uint8_t GATE_VALUE_PRIMARY = 0x2A;  // USB_SEL HIGH (bit 5)
constexpr uint8_t GATE_VALUE_ALT1 = 0x43;     // Alternative from sketch
constexpr uint8_t GATE_VALUE_ALT2 = 0x07;     // Legacy value

// Test state
bool i2c_ok = false;
bool ch422g_detected = false;
bool twai_initialized = false;
uint32_t frames_received = 0;

//=============================================================================
// I2C Utility Functions
//=============================================================================

bool i2c_scan_address(uint8_t addr) {
    Wire.beginTransmission(addr);
    return (Wire.endTransmission() == 0);
}

bool ch422g_write_direct(uint8_t addr, uint8_t value) {
    Wire.beginTransmission(addr);
    Wire.write(value);
    return (Wire.endTransmission(true) == 0);
}

bool ch422g_read_direct(uint8_t addr, uint8_t &value) {
    value = 0;
    int n = Wire.requestFrom((int)addr, 1);
    if (n != 1) return false;
    value = Wire.read();
    return true;
}

//=============================================================================
// Test Functions
//=============================================================================

void test_01_i2c_bus() {
    Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║ TEST 1: I2C Bus Initialization                                ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝");
    
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000);  // 100kHz for stability
    delay(100);
    
    Serial.printf("  SDA Pin: GPIO%d\n", I2C_SDA_PIN);
    Serial.printf("  SCL Pin: GPIO%d\n", I2C_SCL_PIN);
    Serial.println("  Clock:   100 kHz");
    
    // Scan for devices
    Serial.println("\n  Scanning I2C bus...");
    int device_count = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        if (i2c_scan_address(addr)) {
            Serial.printf("    [0x%02X] DETECTED\n", addr);
            device_count++;
            if (addr == CH422G_ADDR_CMD || addr == CH422G_ADDR_OUT) {
                ch422g_detected = true;
            }
        }
    }
    
    if (device_count == 0) {
        Serial.println("    ✗ NO I2C DEVICES FOUND!");
        Serial.println("    → Check SDA/SCL wiring and pull-ups");
        i2c_ok = false;
    } else {
        Serial.printf("    ✓ Found %d I2C device(s)\n", device_count);
        i2c_ok = true;
    }
    
    if (ch422g_detected) {
        Serial.println("    ✓ CH422G I/O expander detected");
    } else {
        Serial.println("    ✗ CH422G NOT detected (critical!)");
        Serial.println("    → CAN transceiver gate control unavailable");
    }
}

void test_02_gpio_pins() {
    Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║ TEST 2: GPIO Pin State (before TWAI init)                     ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝");
    
    pinMode(CAN_TX_PIN, INPUT);
    pinMode(CAN_RX_PIN, INPUT);
    delay(10);
    
    int tx_state = digitalRead(CAN_TX_PIN);
    int rx_state = digitalRead(CAN_RX_PIN);
    
    Serial.printf("  CAN_TX (GPIO%d): %s\n", CAN_TX_PIN, tx_state ? "HIGH" : "LOW");
    Serial.printf("  CAN_RX (GPIO%d): %s\n", CAN_RX_PIN, rx_state ? "HIGH" : "LOW");
    
    // Sample RX pin multiple times to detect activity
    Serial.println("\n  Sampling RX pin (100 samples over 100ms)...");
    int high_count = 0;
    int low_count = 0;
    int transitions = 0;
    int last_state = rx_state;
    
    for (int i = 0; i < 100; i++) {
        int state = digitalRead(CAN_RX_PIN);
        if (state) high_count++; else low_count++;
        if (state != last_state) transitions++;
        last_state = state;
        delayMicroseconds(1000);
    }
    
    Serial.printf("    HIGH:        %d/100\n", high_count);
    Serial.printf("    LOW:         %d/100\n", low_count);
    Serial.printf("    Transitions: %d\n", transitions);
    
    if (high_count == 0) {
        Serial.println("    ✗ RX pin STUCK LOW - transceiver likely disabled!");
    } else if (high_count == 100 && transitions == 0) {
        Serial.println("    ⚠ RX pin stuck HIGH - bus might be quiet or disconnected");
    } else if (transitions > 0) {
        Serial.println("    ✓ RX pin shows activity - good sign!");
    } else {
        Serial.println("    ? RX pin state unclear");
    }
}

void test_03_ch422g_gate_config() {
    Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║ TEST 3: CH422G Gate Configuration                             ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝");
    
    if (!ch422g_detected) {
        Serial.println("  ✗ SKIPPED - CH422G not detected");
        return;
    }
    
    // Try multiple methods to enable the gate
    Serial.println("  Attempting gate enable (multiple strategies)...\n");
    
    // Strategy 1: Write to 0x24 (command register)
    Serial.println("  Strategy 1: Direct write to 0x24");
    bool s1 = ch422g_write_direct(CH422G_ADDR_CMD, GATE_VALUE_PRIMARY);
    Serial.printf("    Write 0x%02X -> 0x24: %s\n", GATE_VALUE_PRIMARY, s1 ? "OK" : "FAIL");
    delay(20);
    
    // Strategy 2: Write to 0x38 (output register)
    Serial.println("\n  Strategy 2: Direct write to 0x38");
    bool s2 = ch422g_write_direct(CH422G_ADDR_OUT, GATE_VALUE_PRIMARY);
    Serial.printf("    Write 0x%02X -> 0x38: %s\n", GATE_VALUE_PRIMARY, s2 ? "OK" : "FAIL");
    delay(20);
    
    // Strategy 3: Two-byte sequence (register + value)
    Serial.println("\n  Strategy 3: Register-style write");
    Wire.beginTransmission(CH422G_ADDR_CMD);
    Wire.write(CH422G_ADDR_OUT);
    Wire.write(GATE_VALUE_PRIMARY);
    bool s3 = (Wire.endTransmission() == 0);
    Serial.printf("    Write [0x38, 0x%02X] -> 0x24: %s\n", GATE_VALUE_PRIMARY, s3 ? "OK" : "FAIL");
    delay(20);
    
    // Strategy 4: Try alternative gate values
    Serial.println("\n  Strategy 4: Alternative gate values");
    ch422g_write_direct(CH422G_ADDR_CMD, GATE_VALUE_ALT1);
    Serial.printf("    Write 0x%02X -> 0x24: attempting...\n", GATE_VALUE_ALT1);
    delay(20);
    ch422g_write_direct(CH422G_ADDR_OUT, GATE_VALUE_ALT1);
    Serial.printf("    Write 0x%02X -> 0x38: attempting...\n", GATE_VALUE_ALT1);
    delay(20);
    
    // Readback verification
    Serial.println("\n  Readback verification:");
    uint8_t val_24 = 0, val_38 = 0;
    bool read_24 = ch422g_read_direct(CH422G_ADDR_CMD, val_24);
    bool read_38 = ch422g_read_direct(CH422G_ADDR_OUT, val_38);
    
    if (read_24) {
        bool usb_sel_24 = (val_24 & (1 << USB_SEL_BIT)) != 0;
        Serial.printf("    0x24 = 0x%02X, USB_SEL (bit %d) = %s\n", 
                      val_24, USB_SEL_BIT, usb_sel_24 ? "HIGH ✓" : "LOW ✗");
    } else {
        Serial.println("    0x24 = READ FAILED");
    }
    
    if (read_38) {
        bool usb_sel_38 = (val_38 & (1 << USB_SEL_BIT)) != 0;
        Serial.printf("    0x38 = 0x%02X, USB_SEL (bit %d) = %s\n", 
                      val_38, USB_SEL_BIT, usb_sel_38 ? "HIGH ✓" : "LOW ✗");
    } else {
        Serial.println("    0x38 = READ FAILED");
    }
    
    delay(50);  // Let hardware settle
}

void test_04_twai_init() {
    Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║ TEST 4: TWAI Driver Initialization                            ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝");
    
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX_PIN, 
        (gpio_num_t)CAN_RX_PIN, 
        TWAI_MODE_NORMAL
    );
    g_config.tx_queue_len = 10;
    g_config.rx_queue_len = 20;
    
    // Try multiple bitrates
    uint32_t bitrates[] = {250000, 500000, 125000};
    const char* names[] = {"250 kbps", "500 kbps", "125 kbps"};
    
    for (int i = 0; i < 3; i++) {
        Serial.printf("\n  Attempting %s...\n", names[i]);
        
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
        if (bitrates[i] == 500000) {
            t_config = TWAI_TIMING_CONFIG_500KBITS();
        } else if (bitrates[i] == 125000) {
            t_config = TWAI_TIMING_CONFIG_125KBITS();
        }
        
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
        
        esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
        
        if (err == ESP_OK) {
            Serial.printf("    ✓ Driver installed successfully at %s\n", names[i]);
            
            err = twai_start();
            if (err == ESP_OK) {
                Serial.println("    ✓ TWAI started successfully");
                twai_initialized = true;
                
                // Get bus status
                twai_status_info_t status;
                if (twai_get_status_info(&status) == ESP_OK) {
                    Serial.println("\n    Bus Status:");
                    Serial.printf("      State:          %d\n", status.state);
                    Serial.printf("      TX Queue:       %lu/%lu\n", status.msgs_to_tx, (uint32_t)g_config.tx_queue_len);
                    Serial.printf("      RX Queue:       %lu/%lu\n", status.msgs_to_rx, (uint32_t)g_config.rx_queue_len);
                    Serial.printf("      TX Error Count: %lu\n", status.tx_error_counter);
                    Serial.printf("      RX Error Count: %lu\n", status.rx_error_counter);
                    Serial.printf("      Bus Errors:     %lu\n", status.bus_error_count);
                }
                
                return;  // Success!
            } else {
                Serial.printf("    ✗ TWAI start failed: %d\n", err);
                twai_driver_uninstall();
            }
        } else {
            Serial.printf("    ✗ Driver install failed: %d\n", err);
        }
    }
    
    Serial.println("\n  ✗ All bitrate attempts FAILED");
}

void test_05_receive_frames() {
    Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║ TEST 5: Live CAN Frame Reception (10 second test)             ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝");
    
    if (!twai_initialized) {
        Serial.println("  ✗ SKIPPED - TWAI not initialized");
        return;
    }
    
    Serial.println("  Listening for CAN frames...");
    Serial.println("  (Make sure your CAN bus has active traffic)\n");
    
    uint32_t start_time = millis();
    uint32_t last_frame_time = 0;
    frames_received = 0;
    
    while (millis() - start_time < 10000) {
        twai_message_t msg;
        esp_err_t err = twai_receive(&msg, pdMS_TO_TICKS(100));
        
        if (err == ESP_OK) {
            frames_received++;
            last_frame_time = millis();
            
            // Print first 5 frames in detail
            if (frames_received <= 5) {
                Serial.printf("  [%04lu] ID: 0x%08lX %s %s DLC: %d Data: ",
                              frames_received,
                              msg.identifier,
                              msg.extd ? "EXT" : "STD",
                              msg.rtr ? "RTR" : "   ",
                              msg.data_length_code);
                
                for (int i = 0; i < msg.data_length_code; i++) {
                    Serial.printf("%02X ", msg.data[i]);
                }
                Serial.println();
            } else if (frames_received % 10 == 0) {
                Serial.printf("  ... %lu frames received\n", frames_received);
            }
        }
    }
    
    Serial.printf("\n  Result: %lu frames received in 10 seconds\n", frames_received);
    
    if (frames_received == 0) {
        Serial.println("  ✗ NO FRAMES RECEIVED!");
        Serial.println("  → Possible causes:");
        Serial.println("    1. CH422G gate not enabled (USB_SEL bit 5 LOW)");
        Serial.println("    2. Wrong bitrate (try 500 kbps if your bus uses it)");
        Serial.println("    3. CAN bus has no active traffic");
        Serial.println("    4. CANH/CANL wiring issue");
        Serial.println("    5. TX/RX pins swapped");
    } else {
        Serial.println("  ✓ SUCCESS! CAN bus is receiving frames");
    }
}

//=============================================================================
// Main Setup and Loop
//=============================================================================

void setup() {
    // Waveshare ESP32-S3-Touch-LCD-7 uses UART0 on GPIO43(TX)/GPIO44(RX)
    // connected to CH343 USB-Serial chip
    Serial.begin(115200);
    delay(3000);  // Longer delay for USB-Serial chip initialization
    
    Serial.println("\n\n\n\n\n");
    Serial.println("╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║                                                                ║");
    Serial.println("║   CAN BUS COMPREHENSIVE DIAGNOSTIC TOOL                        ║");
    Serial.println("║   Waveshare ESP32-S3-Touch-LCD-7                               ║");
    Serial.println("║                                                                ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝");
    Serial.flush();
    delay(500);
    
    // Run all tests
    test_01_i2c_bus();
    test_02_gpio_pins();
    test_03_ch422g_gate_config();
    test_02_gpio_pins();  // Re-check after gate config
    test_04_twai_init();
    test_05_receive_frames();
    
    // Final summary
    Serial.println("\n\n╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║ DIAGNOSTIC SUMMARY                                             ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝");
    
    Serial.printf("  I2C Bus:           %s\n", i2c_ok ? "✓ OK" : "✗ FAIL");
    Serial.printf("  CH422G Detected:   %s\n", ch422g_detected ? "✓ YES" : "✗ NO");
    Serial.printf("  TWAI Initialized:  %s\n", twai_initialized ? "✓ YES" : "✗ NO");
    Serial.printf("  Frames Received:   %lu\n", frames_received);
    
    Serial.println("\n════════════════════════════════════════════════════════════════");
    
    if (frames_received > 0) {
        Serial.println("✓✓✓ CAN BUS IS WORKING! ✓✓✓");
    } else if (!ch422g_detected) {
        Serial.println("✗ CRITICAL: CH422G not found - check I2C wiring");
    } else if (!twai_initialized) {
        Serial.println("✗ CRITICAL: TWAI driver failed to initialize");
    } else {
        Serial.println("✗ CAN transceiver gate may not be enabled");
        Serial.println("  → Review CH422G gate configuration results above");
    }
    
    Serial.println("════════════════════════════════════════════════════════════════\n");
}

void loop() {
    // Continue monitoring in loop
    if (twai_initialized) {
        static uint32_t last_report = 0;
        static uint32_t loop_frames = 0;
        
        twai_message_t msg;
        if (twai_receive(&msg, pdMS_TO_TICKS(100)) == ESP_OK) {
            loop_frames++;
        }
        
        if (millis() - last_report > 5000) {
            Serial.printf("[LIVE] %lu frames received (total: %lu)\n", 
                          loop_frames, frames_received + loop_frames);
            loop_frames = 0;
            last_report = millis();
        }
    }
    
    delay(100);
}
