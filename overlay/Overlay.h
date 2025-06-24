#ifndef OVERLAY_H
#define OVERLAY_H

#include <string>
#include <cstdint>
#include "overlay_wayland_c_bridge.h" // Our clean C header

// By including the full cairo header here, we get the real typedefs
// and avoid any conflicts with the C++ compiler.
#include <cairo/cairo.h>

class Overlay {
public:
    struct Color { uint8_t r, g, b; };
    static const Color RED, GREEN, BLUE, WHITE, BLACK, YELLOW, CYAN, MAGENTA;

    Overlay();
    ~Overlay();

    bool initialize();
    int dispatchEvents();

    void clearBackBuffer();
    void swapBuffers();
    void drawLine(int x1, int y1, int x2, int y2, const Color& color, int thickness = 1);
    void drawBox(int x, int y, int width, int height, const Color& color, int thickness = 1);
    void drawText(int x, int y, const std::string& text, const Color& color, int font_size = 16);

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    bool isInitialized() const { return initialized; }

private:
    wlc_context* ctx = nullptr;

    // These now use the real typedefs from cairo.h and will not conflict.
    cairo_surface_t* cairo_surface = nullptr;
    cairo_t* cairo = nullptr;

    int width = 0;
    int height = 0;
    bool initialized = false;
};

#endif // OVERLAY_H