// overlay/drawing.h
#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno> // Required for errno

#include "shared_data.h"

namespace COLOR {
   using Color = uint32_t;
       inline constexpr Color RED     = 0xFF0000FF;
       inline constexpr Color GREEN   = 0x00FF00FF;
       inline constexpr Color BLUE    = 0x0000FFFF;
       inline constexpr Color WHITE   = 0xFFFFFFFF;
       inline constexpr Color BLACK   = 0x000000FF;
       inline constexpr Color YELLOW  = 0xFFFF00FF;
       inline constexpr Color MAGENTA = 0xFF00FFFF;
       inline constexpr Color CYAN    = 0x00FFFFFF;
       inline constexpr Color ORANGE  = 0xFFA500FF;
       inline constexpr Color PINK    = 0xFF9999FF;

       inline constexpr Color TransparentLightRed    = 0xFF332780; // A light red (Tomato) with 50% transparency
       inline constexpr Color TransparentLightPink   = 0xFFB6C1A0; // A light pink (Light Pink) with 50% transparency
       inline constexpr Color TransparentOrange      = 0xFF4D0080;
       inline constexpr Color TransparentLightGold   = 0xFFD70080; // A classic Gold with 50% transparency
       inline constexpr Color TransparentLightPurple = 0xBA55D380; // A medium, light purple (Medium Orchid) with 50% transparency
       inline constexpr Color TransparentLightBlue   = 0xADD8E680; // A classic Light Blue with 50% transparency
       inline constexpr Color TransparentLightGreen  = 0x90EE9080; // A classic Light Green with 50% transparency
       inline constexpr Color TransparentLightWhite  = 0xFFFFFF80; // A classic Light White with 50% transparency
}

class DrawingContext {
private:
   SharedData* data = nullptr;
   int shm_fd = -1;
   FrameBuffer* write_buffer = nullptr;

   void set_color(auto& cmd_color, COLOR::Color color) {
       cmd_color.a = ((color >> 0)  & 0xFF) / 255.0f;
       cmd_color.b = ((color >> 8)  & 0xFF) / 255.0f;
       cmd_color.g = ((color >> 16) & 0xFF) / 255.0f;
       cmd_color.r = ((color >> 24) & 0xFF) / 255.0f;
   }

public:
   // ===================================================================
   // ====== FIX: ROBUST SHARED MEMORY INITIALIZATION ===================
   // ===================================================================
   DrawingContext() {
       bool a_new_segment_was_created = false;

       // First, try to open an existing segment.
       shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
       if (shm_fd == -1) {
           // If it fails because it doesn't exist, then create it.
           if (errno == ENOENT) {
               std::cout << "Shared memory not found. Creating a new one." << std::endl;
               shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
               if (shm_fd != -1) {
                   a_new_segment_was_created = true;
               }
           }
       } else {
           std::cout << "Connected to existing shared memory." << std::endl;
       }

       // If at this point we still don't have a valid file descriptor, fail.
       if (shm_fd == -1) {
           throw std::runtime_error("Failed to create or open shared memory (shm_open)");
       }

       // If we created a new segment, we MUST set its size.
       if (a_new_segment_was_created) {
           if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
               close(shm_fd);
               shm_unlink(SHM_NAME);
               throw std::runtime_error("Failed to set size of shared memory (ftruncate)");
           }
       }

       // Now, map the memory.
       data = (SharedData*)mmap(nullptr, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
       if (data == MAP_FAILED) {
           close(shm_fd);
           shm_unlink(SHM_NAME); // Unlink on failure
           throw std::runtime_error("Failed to map shared memory (mmap)");
       }

       // FINALLY: Only if we created it, initialize the contents.
       if (a_new_segment_was_created) {
           std::cout << "Initializing new shared memory." << std::endl;
           new (data) SharedData(); // Placement-new to construct the object
           data->frame_id.store(0, std::memory_order_relaxed);
           data->active_buffer_idx.store(0, std::memory_order_relaxed);
       }
       std::cout << "DrawingContext initialized successfully." << std::endl;
   }

   ~DrawingContext() {
       if (data != nullptr) { munmap(data, sizeof(SharedData)); }
       if (shm_fd != -1) { close(shm_fd); }
       // IMPORTANT: This removes the name so it can be cleanly created next time.
       shm_unlink(SHM_NAME);
       std::cout << "DrawingContext cleaned up." << std::endl;
   }

   void begin_frame() {
       int write_idx = !data->active_buffer_idx.load(std::memory_order_acquire);
       write_buffer = &data->buffers[write_idx];
       write_buffer->command_count = 0;
   }
   void end_frame() {
       if (!write_buffer) return;
       data->frame_id.fetch_add(1, std::memory_order_relaxed);
       int write_idx = (write_buffer == &data->buffers[0]) ? 0 : 1;
       data->active_buffer_idx.store(write_idx, std::memory_order_release);
       write_buffer = nullptr;
   }
    //centers text
    void draw_text(float x, float y, const std::string& text, COLOR::Color color) {
       if (x == -1 && y == -1) return;
       if (!write_buffer || write_buffer->command_count >= MAX_COMMANDS) return;

       float text_width = text.length() * 6.0f/*Average Character Width*/;

       float adjusted_x = x - (text_width / 2.0f);

       float adjusted_y = y + (14.0f/*Font Size*/  / 3.5f);

       auto& cmd = write_buffer->commands[write_buffer->command_count++];
       cmd.type = CMD_TEXT;
       cmd.text.x = adjusted_x;
       cmd.text.y = adjusted_y;
       set_color(cmd.text, color);
       strncpy(cmd.text.text, text.c_str(), MAX_TEXT_LEN - 1);
       cmd.text.text[MAX_TEXT_LEN - 1] = '\0';
   }
    void draw_text_uncentered(float x, float y, const std::string& text, COLOR::Color color) {
       if (x == -1 && y == -1) return;

       if (!write_buffer || write_buffer->command_count >= MAX_COMMANDS) return;
       auto& cmd = write_buffer->commands[write_buffer->command_count++];
       cmd.type = CMD_TEXT; cmd.text.x = x; cmd.text.y = y;
       set_color(cmd.text, color);
       strncpy(cmd.text.text, text.c_str(), MAX_TEXT_LEN - 1);
       cmd.text.text[MAX_TEXT_LEN - 1] = '\0';
   }
   void draw_box(float x, float y, float width, float height, float thickness, COLOR::Color color) {
       if (x == -1 && y == -1) return;

       if (!write_buffer || write_buffer->command_count >= MAX_COMMANDS) return;
       auto& cmd = write_buffer->commands[write_buffer->command_count++];
       cmd.type = CMD_BOX; cmd.box.x = x; cmd.box.y = y; cmd.box.width = width; cmd.box.height = height; cmd.box.thickness = thickness;
       set_color(cmd.box, color);
   }
   void draw_line(float x1, float y1, float x2, float y2, float thickness, COLOR::Color color) {
        if (x1 == -1 && y1 == -1 && x2 == -1 && y2 == -1) return;

       if (!write_buffer || write_buffer->command_count >= MAX_COMMANDS) return;
       auto& cmd = write_buffer->commands[write_buffer->command_count++];
       cmd.type = CMD_LINE; cmd.line.x1 = x1; cmd.line.y1 = y1; cmd.line.x2 = x2; cmd.line.y2 = y2; cmd.line.thickness = thickness;
       set_color(cmd.line, color);
   }
};