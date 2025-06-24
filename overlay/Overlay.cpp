#include "Overlay.h"
#include <iostream>
#include <cairo/cairo.h>
#include <chrono>

const Overlay::Color Overlay::RED     = {255, 0, 0};
const Overlay::Color Overlay::GREEN   = {0, 255, 0};
const Overlay::Color Overlay::BLUE    = {0, 0, 255};
const Overlay::Color Overlay::WHITE   = {255, 255, 255};
const Overlay::Color Overlay::BLACK   = {0, 0, 0};
const Overlay::Color Overlay::YELLOW  = {255, 255, 0};
const Overlay::Color Overlay::CYAN    = {0, 255, 255};
const Overlay::Color Overlay::MAGENTA = {255, 0, 255};

Overlay::Overlay() {}

Overlay::~Overlay() {
    if (cairo) cairo_destroy(cairo);
    if (cairo_surface) cairo_surface_destroy(cairo_surface);
    if (ctx) wlc_destroy(ctx);
}

bool Overlay::initialize() {
    ctx = wlc_create();
    if (!ctx) {
        std::cerr << "Failed to initialize Wayland C bridge." << std::endl;
        return false;
    }

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        if (wlc_dispatch(ctx) == -1) {
            std::cerr << "wlc_dispatch returned an error during initialization." << std::endl;
            wlc_destroy(ctx);
            ctx = nullptr;
            return false;
        }

        if (wlc_get_dimensions(ctx, &width, &height)) {
            break;
        }

        auto current_time = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() > 5) {
            std::cerr << "ERROR: Timeout waiting for overlay surface to be configured." << std::endl;
            wlc_destroy(ctx);
            ctx = nullptr;
            return false;
        }
    }

    // --- THE FIX ---
    // The surface is now configured. Immediately perform one full draw cycle
    // to "prime the pump" and give the compositor a valid frame to map.
    std::cout << "Surface configured. Performing initial draw..." << std::endl;
    clearBackBuffer();
    // You could draw something here for the first frame if you wanted.
    // drawText(width / 2, height / 2, "INITIALIZING...", WHITE, 30);
    swapBuffers();
    // --- END OF FIX ---

    initialized = true;
    return true;
}

int Overlay::dispatchEvents() {
    if (!initialized) return 0;
    return wlc_dispatch(ctx);
}

void Overlay::clearBackBuffer() {
    if (!ctx) return;
    wlc_buffer* buffer = wlc_get_draw_buffer(ctx);
    if (!buffer) return;

    if (cairo_surface && cairo_image_surface_get_data(cairo_surface) != buffer->data) {
        cairo_destroy(cairo);
        cairo_surface_destroy(cairo_surface);
        cairo = nullptr;
        cairo_surface = nullptr;
    }

    if (!cairo_surface) {
        cairo_surface = cairo_image_surface_create_for_data(
            (unsigned char*)buffer->data,
            CAIRO_FORMAT_ARGB32,
            buffer->width,
            buffer->height,
            buffer->stride
        );
        cairo = cairo_create(cairo_surface);
    }

    cairo_set_source_rgba(cairo, 0, 0, 0, 0);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);
    cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
}

void Overlay::swapBuffers() {
    if (!ctx) return;
    wlc_swap_buffers(ctx);
}

void Overlay::drawLine(int x1, int y1, int x2, int y2, const Color& color, int thickness) {
    if (!cairo) return;
    cairo_set_source_rgb(cairo, color.r / 255.0, color.g / 255.0, color.b / 255.0);
    cairo_set_line_width(cairo, thickness);
    cairo_move_to(cairo, x1, y1);
    cairo_line_to(cairo, x2, y2);
    cairo_stroke(cairo);
}

void Overlay::drawBox(int x, int y, int w, int h, const Color& color, int thickness) {
    if (!cairo) return;
    cairo_set_source_rgb(cairo, color.r / 255.0, color.g / 255.0, color.b / 255.0);
    cairo_set_line_width(cairo, thickness);
    cairo_rectangle(cairo, x, y, w, h);
    cairo_stroke(cairo);
}

void Overlay::drawText(int x, int y, const std::string& text, const Color& color, int font_size) {
    if (!cairo) return;
    cairo_set_source_rgb(cairo, color.r / 255.0, color.g / 255.0, color.b / 255.0);
    cairo_select_font_face(cairo, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo, font_size);
    cairo_move_to(cairo, x, y);
    cairo_show_text(cairo, text.c_str());
}