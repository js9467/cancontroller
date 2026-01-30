/**
 * @file app_state.cpp
 * Application state management implementation
 */

#include "app_state.h"
#include <Arduino.h>

// Singleton instance
AppState& AppState::getInstance() {
    static AppState instance;
    return instance;
}

// Private constructor
AppState::AppState() 
    : current_screen(Screen::HOME)
    , previous_screen(Screen::HOME)
    , uptime_ms(0)
{
    // Initialize all windows as closed
    for (int i = 0; i < 4; i++) {
        window_states[i] = WindowState::CLOSED;
    }

    // Initialize all doors as unlocked
    for (int i = 0; i < 4; i++) {
        lock_states[i] = LockState::UNLOCKED;
    }
}

// ========== SCREEN MANAGEMENT ==========

void AppState::navigateToScreen(Screen screen) {
    if (current_screen == screen) return;

    previous_screen = current_screen;
    current_screen = screen;

    Serial.printf("Navigation: %d -> %d\n", (int)previous_screen, (int)current_screen);

    notifyScreenChange();
}

void AppState::notifyScreenChange() {
    if (screen_change_callback) {
        screen_change_callback(current_screen);
    }
}

// ========== WINDOW STATE ==========

WindowState AppState::getWindowState(uint8_t window_id) const {
    if (window_id >= 4) return WindowState::UNKNOWN;
    return window_states[window_id];
}

void AppState::setWindowState(uint8_t window_id, WindowState state) {
    if (window_id >= 4) return;

    if (window_states[window_id] != state) {
        window_states[window_id] = state;
        Serial.printf("Window %d: state=%d\n", window_id, (int)state);
        notifyWindowStateChange();
    }
}

void AppState::openWindow(uint8_t window_id) {
    if (window_id >= 4) return;

    // Simulate window opening sequence
    setWindowState(window_id, WindowState::OPENING);

    // In real implementation, this would interface with CAN/J1939
    // For now, just simulate with a delay (would be handled async in real app)
    Serial.printf("Opening window %d...\n", window_id);
    
    // After some time, mark as open (this is synchronous for demo)
    // In production, you'd use a timer or CAN message to update
}

void AppState::closeWindow(uint8_t window_id) {
    if (window_id >= 4) return;

    // Simulate window closing sequence
    setWindowState(window_id, WindowState::CLOSING);

    // In real implementation, this would interface with CAN/J1939
    Serial.printf("Closing window %d...\n", window_id);
}

void AppState::notifyWindowStateChange() {
    if (window_state_callback) {
        window_state_callback();
    }
}

// ========== LOCK STATE ==========

LockState AppState::getLockState(uint8_t door_id) const {
    if (door_id >= 4) return LockState::UNLOCKED;
    return lock_states[door_id];
}

void AppState::setLockState(uint8_t door_id, LockState state) {
    if (door_id >= 4) return;

    if (lock_states[door_id] != state) {
        lock_states[door_id] = state;
        Serial.printf("Door %d: %s\n", door_id, 
                     state == LockState::LOCKED ? "LOCKED" : "UNLOCKED");
        notifyLockStateChange();
    }
}

void AppState::lockAll() {
    Serial.println("Locking all doors...");
    for (int i = 0; i < 4; i++) {
        setLockState(i, LockState::LOCKED);
    }
}

void AppState::unlockAll() {
    Serial.println("Unlocking all doors...");
    for (int i = 0; i < 4; i++) {
        setLockState(i, LockState::UNLOCKED);
    }
}

void AppState::notifyLockStateChange() {
    if (lock_state_callback) {
        lock_state_callback();
    }
}
