#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <linux/input.h>
#include <linux/input-event-codes.h>

class InputManager {
public:
    /**
     * @brief Constructs the manager. Scans for all input devices,
     *        sets up the virtual mouse, and starts monitoring threads.
     */
    InputManager();

    /**
     * @brief Destructor. Stops all monitoring threads and cleans up resources.
     */
    ~InputManager();

    // --- Input Monitoring API ---

    /**
     * @brief Checks if a specific key is currently held down on ANY monitored keyboard.
     * @param keyCode The code from <linux/input-event-codes.h> (e.g., KEY_W, KEY_KP1).
     * @return True if the key is pressed, false otherwise.
     */
    bool isKeyDown(int keyCode) const;

    /**
     * @brief Gets the accumulated relative mouse movement since the last call.
     *        This function resets the internal deltas to zero after reading.
     * @param dx Reference to store the change in X.
     * @param dy Reference to store the change in Y.
     */
    void getMouseDelta(int& dx, int& dy);

    // --- Virtual Device Output API ---

    /**
     * @brief Moves the virtual mouse pointer.
     * @param dx The relative change in the X-axis.
     * @param dy The relative change in the Y-axis.
     */
    void moveMouseRelative(int dx, int dy);
    
    /**
     * @brief Simulates a left mouse button click (down and up).
     */
    void clickLeft();
    
    /**
     * @brief Simulates pressing the left mouse button down.
     */
    void leftButtonDown();
    
    /**
     * @brief Simulates releasing the left mouse button.
     */
    void leftButtonUp();


    /**
     * @brief Checks if the virtual mouse was initialized successfully.
     */
    bool isVirtualMouseInitialized() const;


    // Disable copy/move semantics for this complex class
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

private:
    // --- Core Methods ---
    void scanAndOpenDevices();
    void monitorDevice(int fd);
    void setupUinput();
    void emitUinputEvent(int type, int code, int value);

    // --- State ---
    std::vector<int> device_fds;
    std::vector<std::thread> monitor_threads;
    std::atomic<bool> running;

    // --- Shared State (Protected by Mutex) ---
    mutable std::mutex state_mutex;
    std::vector<bool> key_states;
    int delta_x;
    int delta_y;

    // --- UInput (Virtual Device) State ---
    int uinput_fd;
    bool uinput_initialized;
};

#endif // INPUT_MANAGER_H