// overlay/Overlay.h
#pragma once

#include <chrono>
#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include "shared_data.h" // Make sure this path is correct

class Overlay {
public:
    Overlay();
    ~Overlay();
    int run(int argc, char** argv);

private:
    void setup_window();
    void connect_shared_memory();

    // The GTK signal handlers
    static void on_activate(GtkApplication* app, gpointer user_data);
    static int on_command_line(GApplication* app, GApplicationCommandLine* cmdline, gpointer user_data);

    // FIX: Add a handler for the "realize" signal to set the input shape
    static void on_realize(GtkWidget* widget, gpointer user_data);

    static gboolean tick_callback(gpointer user_data);
    static void draw_func(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer user_data);

    void queue_redraw();

    // Member variables
    int target_monitor_index = 0;
    GtkApplication* app = nullptr;
    GtkWidget* window = nullptr;
    GtkWidget* drawing_area = nullptr;
    SharedData* shared_data = nullptr;
    uint64_t last_drawn_frame_id = 0;

    std::chrono::steady_clock::time_point last_fps_time = std::chrono::steady_clock::now();
    int frame_counter = 0;
    float current_fps = 0.0f;
};