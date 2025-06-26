// common/shared_data.h
#pragma once

#include <atomic>
#include <cstdint>

// A unique name for our shared memory segment.
#define SHM_NAME "/dan_sot_shm"

// The maximum number of draw commands per frame. Your 12000 writes/sec is
// a frequency, not a per-frame count. Let's assume a max of 2048 commands
// per rendered frame, which is plenty for a 120Hz refresh rate.
#define MAX_COMMANDS 2048
#define MAX_TEXT_LEN 64

enum DrawCommandType {
    CMD_BOX,
    CMD_TEXT,
    CMD_LINE
};

struct DrawCommandData {
    DrawCommandType type;
    union {
        struct {
            float x, y, width, height, thickness;
            float r, g, b, a;
        } box;

        struct {
            float x, y;
            float r, g, b, a;
            char text[MAX_TEXT_LEN];
        } text;

        struct {
            float x1, y1, x2, y2, thickness;
            float r, g, b, a;
        } line;
    };
};

// This structure holds all the commands for a single frame.
struct FrameBuffer {
    uint32_t command_count;
    DrawCommandData commands[MAX_COMMANDS];
};

// This is the top-level structure that will be in shared memory.
// It implements double buffering.
struct SharedData {
    // Incremented by the cheat for each new frame. The overlay checks this
    // to see if a redraw is needed.
    std::atomic<uint64_t> frame_id;

    // The index (0 or 1) of the buffer that the overlay should READ from.
    // The cheat writes to the *other* buffer.
    std::atomic<int> active_buffer_idx;

    // The two buffers for drawing.
    FrameBuffer buffers[2];
};