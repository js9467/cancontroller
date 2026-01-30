/**
 * @file app_state.h
 * Global application state management
 * 
 * Singleton pattern for managing vehicle state (windows, locks, etc.)
 * Provides callbacks for state changes and screen navigation
 */

#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>
#include <functional>

// Forward declarations
typedef struct _lv_obj_t lv_obj_t;

/**
 * @brief Current active screen
 */
enum class Screen {
    HOME,
    WINDOWS,
    LOCKS,
    RUNNING_BOARDS
};

/**
 * @brief Window state for each window
 */
enum class WindowState {
    UNKNOWN,
    CLOSED,
    OPENING,
    OPEN,
    CLOSING
};

/**
 * @brief Lock state for doors
 */
enum class LockState {
    LOCKED,
    UNLOCKED
};

/**
 * @brief Application state singleton
 */
class AppState {
public:
    // Callback types
    using StateCallback = std::function<void()>;
    using ScreenCallback = std::function<void(Screen)>;

    /**
     * @brief Get singleton instance
     */
    static AppState& getInstance();

    // Prevent copying
    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;

    // ========== SCREEN MANAGEMENT ==========
    /**
     * @brief Get current active screen
     */
    Screen getCurrentScreen() const { return current_screen; }

    /**
     * @brief Navigate to a screen
     */
    void navigateToScreen(Screen screen);

    /**
     * @brief Register callback for screen changes
     */
    void setScreenChangeCallback(ScreenCallback callback) {
        screen_change_callback = callback;
    }

    // ========== WINDOW STATE ==========
    /**
     * @brief Get window state
     * @param window_id 0=driver, 1=passenger, 2=rear_left, 3=rear_right
     */
    WindowState getWindowState(uint8_t window_id) const;

    /**
     * @brief Set window state
     */
    void setWindowState(uint8_t window_id, WindowState state);

    /**
     * @brief Start opening a window (simulated for now)
     */
    void openWindow(uint8_t window_id);

    /**
     * @brief Start closing a window (simulated for now)
     */
    void closeWindow(uint8_t window_id);

    /**
     * @brief Register callback for window state changes
     */
    void setWindowStateCallback(StateCallback callback) {
        window_state_callback = callback;
    }

    // ========== LOCK STATE ==========
    /**
     * @brief Get lock state
     * @param door_id 0=driver, 1=passenger, 2=rear_left, 3=rear_right
     */
    LockState getLockState(uint8_t door_id) const;

    /**
     * @brief Set lock state
     */
    void setLockState(uint8_t door_id, LockState state);

    /**
     * @brief Lock all doors
     */
    void lockAll();

    /**
     * @brief Unlock all doors
     */
    void unlockAll();

    /**
     * @brief Register callback for lock state changes
     */
    void setLockStateCallback(StateCallback callback) {
        lock_state_callback = callback;
    }

    // ========== SYSTEM STATE ==========
    /**
     * @brief Get system uptime in milliseconds
     */
    uint32_t getUptime() const { return uptime_ms; }

    /**
     * @brief Update system uptime (call periodically)
     */
    void updateUptime(uint32_t ms) { uptime_ms = ms; }

private:
    AppState();  // Private constructor for singleton

    // Screen state
    Screen current_screen;
    Screen previous_screen;
    ScreenCallback screen_change_callback;

    // Window states (4 windows)
    WindowState window_states[4];
    StateCallback window_state_callback;

    // Lock states (4 doors)
    LockState lock_states[4];
    StateCallback lock_state_callback;

    // System state
    uint32_t uptime_ms;

    // Helper to notify callbacks
    void notifyWindowStateChange();
    void notifyLockStateChange();
    void notifyScreenChange();
};

#endif // APP_STATE_H
