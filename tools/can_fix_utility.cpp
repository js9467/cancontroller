/*
  CAN Fix Utility - Force CH422G into CAN mode using raw I2C
  
  This tool directly sets the CH422G EXIO5 pin HIGH to enable CAN transceiver.
  Upload this if CAN stops working to diagnose and fix the hardware configuration.
  
  Based on working configuration from user's sniffer sketch.
*/

#include <Arduino.h>
#include <Wire.h>
#include "driver/twai.h"

// CH422G Configuration
#define CH422G_ADDR 0x20
#define I2C_SDA     8
#define I2C_SCL     9
#define USB_SEL_BIT 5   // EXIO5 - must be HIGH for CAN mode

// CAN Configuration
#define CAN_TX_PIN 20
#define CAN_RX_PIN 19

bool can_enabled = false;

void forceCAN_RAW() {
    Serial.println("\n[FIX] Using RAW I2C to force CAN mode...");
    
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(50);
    
    // Method 1: Single bit set (like user's working sketch)
    Wire.beginTransmission(CH422G_ADDR);
    Wire.write(1 << USB_SEL_BIT);  // EXIO5 HIGH = CAN mode
    uint8_t result = Wire.endTransmission();
    
    Serial.printf("[FIX] CH422G write result: %d (0=success)\n", result);
    Serial.println("[FIX] EXIO5 set HIGH for CAN transceiver");
    
    delay(100);
    
    // Verify by reading back
    Wire.requestFrom(CH422G_ADDR, 1);
    if (Wire.available()) {
        uint8_t state = Wire.read();
        Serial.printf("[FIX] CH422G readback: 0x%02X\n", state);
        Serial.printf("[FIX] EXIO5 state: %s\n", (state & (1 << USB_SEL_BIT)) ? "HIGH ✓" : "LOW ✗");
    }
}

void testCANInit() {
    Serial.println("\n[TEST] Initializing CAN...");
    
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX_PIN,
        (gpio_num_t)CAN_RX_PIN,
        TWAI_MODE_LISTEN_ONLY
    );
    
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    esp_err_t install_result = twai_driver_install(&g_config, &t_config, &f_config);
    Serial.printf("[TEST] TWAI install: %s\n", install_result == ESP_OK ? "OK ✓" : "FAILED ✗");
    
    if (install_result == ESP_OK) {
        esp_err_t start_result = twai_start();
        Serial.printf("[TEST] TWAI start: %s\n", start_result == ESP_OK ? "OK ✓" : "FAILED ✗");
        
        if (start_result == ESP_OK) {
            can_enabled = true;
            Serial.println("[TEST] ✓ CAN is working!");
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println("\n\n========================================");
    Serial.println("   CAN FIX UTILITY - Hardware Diagnostic");
    Serial.println("========================================\n");
    
    // Step 1: Force CAN mode using raw I2C
    forceCAN_RAW();
    
    // Step 2: Try to initialize CAN
    testCANInit();
    
    Serial.println("\n========================================");
    if (can_enabled) {
        Serial.println("   RESULT: CAN is WORKING ✓");
        Serial.println("   Monitoring for CAN frames...");
    } else {
        Serial.println("   RESULT: CAN FAILED ✗");
        Serial.println("   Hardware issue detected!");
    }
    Serial.println("========================================\n");
}

void loop() {
    if (!can_enabled) {
        delay(5000);
        Serial.println("[INFO] CAN not enabled - trying fix again...");
        forceCAN_RAW();
        testCANInit();
        return;
    }
    
    // Monitor for CAN frames
    twai_message_t msg;
    if (twai_receive(&msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
        Serial.printf("[%lu] CAN RX: ID=0x%08lX DLC=%u DATA=", 
                      millis(), msg.identifier, msg.data_length_code);
        for (int i = 0; i < msg.data_length_code; i++) {
            Serial.printf("%02X ", msg.data[i]);
        }
        Serial.println();
    }
}
