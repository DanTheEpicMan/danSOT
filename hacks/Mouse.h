#ifndef MOUSE_H
#define MOUSE_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

// This header contains all the KEY_* and BTN_* definitions
#include <linux/input-event-codes.h>

class Mouse {
public:
    Mouse();
    ~Mouse();

    // --- Original Output Functions (UInput) ---
    void moveRelative(int dx, int dy);
    void leftButtonDown();
    void leftButtonUp();
    void rightButtonDown();
    void rightButtonUp();
    bool isInitialized() const;

    // --- NEW: Input Monitoring Functions ---

    /**
     * @brief Starts monitoring a specific input device for key presses.
     *        This should be a device that acts like a keyboard, like the side
     *        buttons on a gaming mouse.
     * @param device_name_filter A substring to identify your device (e.g., "Razer", "Corsair", "Naga").
     * @return True if the device was found and monitoring started, false otherwise.
     */
    bool startMonitoring(const std::string& device_name_filter);

    /**
     * @brief Checks if a specific key/button on the monitored device is currently held down.
     * @param keyCode The code for the key (e.g., KEY_KP1 for numpad 1, KEY_KP2 for numpad 2).
     * @return True if the key is pressed, false otherwise.
     */
    bool isKeyDown(int keyCode) const;

    // Disable copy/move
    Mouse(const Mouse&) = delete;
    Mouse& operator=(const Mouse&) = delete;

private:
    // --- UInput (Output) State ---
    int uinput_fd = -1;
    bool initialized = false;
    void emit(int type, int code, int value);

    // --- NEW: Input Monitoring State ---
    int input_fd = -1; // File descriptor for listening to the input device
    std::atomic<bool> monitoring;
    std::thread monitor_thread;
    mutable std::mutex key_state_mutex;
    std::vector<bool> key_states;

    std::string findInputDevice(const std::string& filter);
    void monitorLoop();
};

#endif // MOUSE_H