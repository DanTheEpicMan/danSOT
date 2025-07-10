# Overlay
This is a special overlay that allows it ro run in a seperate process. This is usefull because it allows the overlay to run as non-root (needed in wayland).
While our main application is running as root, the overlay can run as non-root. This allows us to read memory without being detected by the game.

# While Programming
During programming only drawing.h and shared_data.h need to be in the cheat process, everything else will compile itself into an app. This app will then be run separately and the cheat will communicate with the overlay.

Example code from GPT:
```cpp
//NOTE: Uses wrappers for underlying functions for drawing (ie theres a more manual way but this is cleaner and easier to use)
// main.cpp (The "Cheat")

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>      // For std::sin and std::cos
#include <string>     // For std::string and std::to_string

// Include our new, clean API for drawing
#include "cheat_api/drawing.h"

int main() {
    try {
        // Create the drawing context. This object handles all the complex
        // shared memory setup in its constructor and all the cleanup in
        // its destructor (when it goes out of scope at the end of main).
        // This is the RAII (Resource Acquisition Is Initialization) pattern.
        DrawingContext ctx;

        std::cout << "Cheat running. Drawing to overlay. Press Ctrl+C to exit." << std::endl;
        std::cout << "The DrawingContext destructor will handle cleanup automatically." << std::endl;

        uint64_t frame_counter = 0;

        // The main application loop.
        while (true) {
            // 1. Signal the start of a new frame's worth of drawing commands.
            //    This clears the off-screen buffer we're about to write to.
            ctx.begin_frame();

            // --- Define some colors for convenience ---
            Color red   = {1.0f, 0.0f, 0.0f, 1.0f};
            Color green = {0.0f, 1.0f, 0.0f, 1.0f};
            Color blue  = {0.0f, 0.5f, 1.0f, 1.0f};
            Color white = {1.0f, 1.0f, 1.0f, 1.0f};

            // --- Generate draw commands using the simple API ---

            // Draw a spinning line in the center of a 1920x1080 screen
            float center_x = 1920.0f / 2.0f;
            float center_y = 1080.0f / 2.0f;
            float radius = 150.0f;
            float angle = frame_counter / 50.0f;
            float line_end_x = center_x + radius * cos(angle);
            float line_end_y = center_y + radius * sin(angle);
            ctx.draw_line(center_x, center_y, line_end_x, line_end_y, 3.0f, blue);

            // Draw a box that follows the end of the line
            ctx.draw_box(line_end_x - 10, line_end_y - 10, 20.0f, 20.0f, 2.0f, red);

            // Draw text showing the frame count and FPS (a simple estimation)
            std::string text_content = "Frame: " + std::to_string(frame_counter) + " | C++20 | Wayland Overlay";
            ctx.draw_text(50.0f, 50.0f, text_content, white);

            // Draw another line at the bottom of the screen
            ctx.draw_line(0, 1000, 1920, 1000, 1.0f, green);


            // 2. Signal that we are done drawing for this frame.
            //    This will "swap the buffers", making our commands visible
            //    to the overlay application on its next refresh.
            ctx.end_frame();

            // Increment our counter for animations
            frame_counter++;

            // Wait for a short period to aim for a specific update rate.
            // 8ms is ~125 FPS. 16ms is ~62.5 FPS.
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }

    } catch (const std::runtime_error& e) {
        // If the DrawingContext constructor failed (e.g., permissions error on /dev/shm),
        // it will throw an exception. We catch it here for a clean exit.
        std::cerr << "Fatal Error in Cheat Application: " << e.what() << std::endl;
        return 1;
    }

    // The 'ctx' object goes out of scope here, and its destructor is called,
    // which automatically unmaps and unlinks the shared memory.
    return 0;
}
```

```cpp
//NOTE: MANUAL WITHOUT USING AUTOMATIC FUNCTIONS
// main.cpp (The "Cheat")
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstring>

// POSIX Shared Memory
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "common/shared_data.h"

// Global pointer to our shared memory data
SharedData* shared_data = nullptr;

// Signal handler to clean up shared memory on exit (Ctrl+C)
void cleanup_shm(int signum) {
    std::cout << "\nCaught signal " << signum << ". Cleaning up shared memory." << std::endl;
    if (shared_data != nullptr) {
        munmap(shared_data, sizeof(SharedData));
    }
    shm_unlink(SHM_NAME);
    exit(0);
}

int main() {
    // Register signal handler for cleanup
    signal(SIGINT, cleanup_shm);

    // 1. Create the shared memory segment
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // 2. Set the size of the shared memory
    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        return 1;
    }

    // 3. Map the shared memory into this process's address space
    shared_data = (SharedData*)mmap(nullptr, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        return 1;
    }

    // 4. Initialize the shared data structure
    new (shared_data) SharedData(); // Use placement new to construct atomics
    shared_data->frame_id.store(0);
    shared_data->active_buffer_idx.store(0);

    std::cout << "Cheat running. Writing to shared memory. Press Ctrl+C to exit." << std::endl;

    // Main loop to generate draw commands
    while (true) {
        // We want to write at ~120 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(8));

        // Determine which buffer is INACTIVE (the one we can write to)
        int write_idx = !shared_data->active_buffer_idx.load(std::memory_order_acquire);
        FrameBuffer* current_buffer = &shared_data->buffers[write_idx];
        current_buffer->command_count = 0;

        // --- Generate some example draw commands ---
        uint64_t frame = shared_data->frame_id.load(std::memory_order_relaxed);

        // A moving box
        DrawCommandData& box_cmd = current_buffer->commands[current_buffer->command_count++];
        box_cmd.type = CMD_BOX;
        box_cmd.box.x = 200 + 100 * sin(frame / 60.0);
        box_cmd.box.y = 200 + 100 * cos(frame / 60.0);
        box_cmd.box.width = 50;
        box_cmd.box.height = 50;
        box_cmd.box.thickness = 2.0;
        box_cmd.box.r = 1.0; box_cmd.box.g = 0.0; box_cmd.box.b = 0.0; box_cmd.box.a = 1.0;

        // A line
        DrawCommandData& line_cmd = current_buffer->commands[current_buffer->command_count++];
        line_cmd.type = CMD_LINE;
        line_cmd.line.x1 = 300; line_cmd.line.y1 = 300;
        line_cmd.line.x2 = 600; line_cmd.line.y2 = 300 + 50 * sin(frame / 30.0);
        line_cmd.line.thickness = 3.0;
        line_cmd.line.r = 0.0; line_cmd.line.g = 1.0; line_cmd.line.b = 0.0; line_cmd.line.a = 1.0;

        // Some text with the frame count
        DrawCommandData& text_cmd = current_buffer->commands[current_buffer->command_count++];
        text_cmd.type = CMD_TEXT;
        text_cmd.text.x = 50;
        text_cmd.text.y = 50;
        text_cmd.text.r = 1.0; text_cmd.text.g = 1.0; text_cmd.text.b = 1.0; text_cmd.text.a = 1.0;
        snprintf(text_cmd.text.text, MAX_TEXT_LEN, "Frame: %lu", frame);

        // --- SWAP BUFFERS ---
        // 1. Increment the frame ID to signal new data is ready.
        shared_data->frame_id.fetch_add(1, std::memory_order_relaxed);
        // 2. Atomically flip the active buffer index. The overlay will now read what we just wrote.
        shared_data->active_buffer_idx.store(write_idx, std::memory_order_release);
    }

    // This part is unreachable due to the loop and signal handler, but good practice.
    cleanup_shm(0);
    return 0;
}
```