// overlay/Overlay.cpp
#include "Overlay.h"
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cairo/cairo.h>
#include <string> // <<< DEBUGGING >>> For creating debug strings

#define debug false

Overlay::Overlay() = default;

Overlay::~Overlay() {
   if (shared_data != nullptr && shared_data != MAP_FAILED) {
       munmap(shared_data, sizeof(SharedData));
   }
}

int Overlay::run(int argc, char** argv) {
   app = gtk_application_new("com.dan.sot", G_APPLICATION_HANDLES_COMMAND_LINE);
   g_signal_connect(app, "activate", G_CALLBACK(on_activate), this);
   g_signal_connect(app, "command-line", G_CALLBACK(on_command_line), this);
   int status = g_application_run(G_APPLICATION(app), argc, argv);
   g_object_unref(app);
   return status;
}

int Overlay::on_command_line(GApplication* app, GApplicationCommandLine* cmdline, gpointer user_data) {
   auto* self = static_cast<Overlay*>(user_data);
   gchar** argv = g_application_command_line_get_arguments(cmdline, nullptr);
   if (argv && argv[1]) {
       try {
           self->target_monitor_index = std::stoi(argv[1]);
           std::cout << "Monitor index set from command line: " << self->target_monitor_index << std::endl;
       } catch (...) {
           std::cerr << "Invalid monitor index provided. Using default (0)." << std::endl;
           self->target_monitor_index = 0;
       }
   }
   g_strfreev(argv);
   g_application_activate(app);
   return 0;
}

void Overlay::on_activate(GtkApplication* app, gpointer user_data) {
   auto* self = static_cast<Overlay*>(user_data);
   self->connect_shared_memory();
   self->setup_window();
}

void Overlay::on_realize(GtkWidget* widget, gpointer user_data) {
    GdkSurface* surface = gtk_native_get_surface(gtk_widget_get_native(widget));
    cairo_region_t* empty_region = cairo_region_create();
    gdk_surface_set_input_region(surface, empty_region);
    cairo_region_destroy(empty_region);
}

void Overlay::connect_shared_memory() {
   int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
   if (shm_fd == -1) {
       perror("shm_open failed in overlay");
       exit(1);
   }
   shared_data = (SharedData*)mmap(nullptr, sizeof(SharedData), PROT_READ, MAP_SHARED, shm_fd, 0);
   if (shared_data == MAP_FAILED) {
       perror("mmap failed in overlay");
       exit(1);
   }
   close(shm_fd);
   std::cout << "Overlay connected to shared memory." << std::endl;
}

void Overlay::setup_window() {
   window = gtk_application_window_new(app);
   gtk_window_set_title(GTK_WINDOW(window), "DanSOT Overlay");

   GtkCssProvider* provider = gtk_css_provider_new();
   gtk_css_provider_load_from_string(provider, "window { background-color: transparent; }");
   gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   g_object_unref(provider);

   gtk_layer_init_for_window(GTK_WINDOW(window));

   GdkDisplay *display = gdk_display_get_default();
   GListModel *monitors = gdk_display_get_monitors(display);
   if (target_monitor_index >= 0 && (guint)target_monitor_index < g_list_model_get_n_items(monitors)) {
       GdkMonitor *monitor = (GdkMonitor*)g_list_model_get_item(monitors, target_monitor_index);
       gtk_layer_set_monitor(GTK_WINDOW(window), monitor);
   } else {
       std::cerr << "Warn: Monitor index " << target_monitor_index << " is invalid. Defaulting to primary." << std::endl;
   }
   g_object_unref(monitors);

   gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
   gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
   gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
   gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
   gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
   gtk_layer_set_keyboard_mode(GTK_WINDOW(window), GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);

   drawing_area = gtk_drawing_area_new();
   gtk_window_set_child(GTK_WINDOW(window), drawing_area);
   gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), draw_func, this, nullptr);
   g_signal_connect(drawing_area, "realize", G_CALLBACK(on_realize), this);

   // Request updates at ~120 Hz
   g_timeout_add(1, tick_callback, this); // was g_timeout_add(8, tick_callback, this);
   gtk_window_present(GTK_WINDOW(window));
}

void Overlay::queue_redraw() {
   if (drawing_area) {
       gtk_widget_queue_draw(drawing_area);
   }
}

gboolean Overlay::tick_callback(gpointer user_data) {
   auto* self = static_cast<Overlay*>(user_data);
   if (!self->shared_data) return G_SOURCE_CONTINUE;

   // <<< DEBUGGING >>> A counter to prevent spamming the console
   static int tick_count = 0;

   // Using memory_order_acquire to be safe, ensuring we see the new frame_id
   uint64_t current_frame_id = self->shared_data->frame_id.load(std::memory_order_acquire);

#if debug
   <<< DEBUGGING >>> Print status periodically
   if (tick_count++ % 120 == 0) { // Print once per second (approx)
       g_print("Tick... Shared Frame ID: %lu, My Last Drawn Frame ID: %lu\n",
               current_frame_id, self->last_drawn_frame_id);
   }
#endif

   if (current_frame_id > self->last_drawn_frame_id) {
#if debug
       // <<< DEBUGGING >>> Log when we detect a new frame
       if (tick_count % 120 == 1) { // Only print this right after the status print
          g_print(">>> New frame detected! Queueing redraw.\n");
       }
#endif
       self->last_drawn_frame_id = current_frame_id;
       self->queue_redraw();
   }
   return G_SOURCE_CONTINUE;
}

void Overlay::draw_func(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer user_data) {
   // Clear the canvas
   cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
   cairo_paint(cr);
   cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

   auto* self = static_cast<Overlay*>(user_data);
   if (!self->shared_data) return;

   // This ensures that we read the buffer index *after* it has been updated by the writer
   int read_idx = self->shared_data->active_buffer_idx.load(std::memory_order_acquire);
   const FrameBuffer* buffer_to_draw = &self->shared_data->buffers[read_idx];

   // Clamp command count to a safe maximum
   uint32_t count = buffer_to_draw->command_count;
   if (count > MAX_COMMANDS) count = MAX_COMMANDS;

#if debug
   // <<< DEBUGGING >>> Draw debug info directly on the screen
   char debug_text[128];
   snprintf(debug_text, sizeof(debug_text), "Frame: %lu | Buf Idx: %d | Cmds: %u",
            self->last_drawn_frame_id, read_idx, count);

   cairo_set_source_rgba(cr, 0, 1, 0, 1); // Bright Green
   cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
   cairo_set_font_size(cr, 16.0);
   cairo_move_to(cr, 10, 20); // Top-left corner
   cairo_show_text(cr, debug_text);

   // <<< DEBUGGING >>> Print to console when draw_func is called.
   static int draw_count = 0;
   if (draw_count++ % 120 == 0) { // Print periodically
       g_print("Draw... Reading buffer %d with %u commands.\n", read_idx, count);
   }
   // <<< END DEBUGGING >>>
#endif

   // Now, render the actual commands from the buffer
   for (uint32_t i = 0; i < count; ++i) {
       const auto& cmd = buffer_to_draw->commands[i];
       switch (cmd.type) {
           case CMD_BOX:
               cairo_set_source_rgba(cr, cmd.box.r, cmd.box.g, cmd.box.b, cmd.box.a);
               cairo_set_line_width(cr, cmd.box.thickness);
               cairo_rectangle(cr, cmd.box.x, cmd.box.y, cmd.box.width, cmd.box.height);
               cairo_stroke(cr);
               break;
           case CMD_LINE:
               cairo_set_source_rgba(cr, cmd.line.r, cmd.line.g, cmd.line.b, cmd.line.a);
               cairo_set_line_width(cr, cmd.line.thickness);
               cairo_move_to(cr, cmd.line.x1, cmd.line.y1);
               cairo_line_to(cr, cmd.line.x2, cmd.line.y2);
               cairo_stroke(cr);
               break;
           case CMD_TEXT:
               cairo_set_source_rgba(cr, cmd.text.r, cmd.text.g, cmd.text.b, cmd.text.a);
               cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
               cairo_set_font_size(cr, 14.0);
               cairo_move_to(cr, cmd.text.x, cmd.text.y);
               cairo_show_text(cr, cmd.text.text);
               break;
       }
   }
}