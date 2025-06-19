#ifndef OVERLAY_H
#define OVERLAY_H

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <string>

class Overlay {
public:
    // Nested Color struct for better namespacing
    struct Color {
        int r, g, b;
    };

    // Pre-defined static colors for convenience
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color WHITE;
    static const Color BLACK;
    static const Color YELLOW;
    static const Color CYAN;
    static const Color MAGENTA;


    Overlay();
    ~Overlay();

    bool initialize(const std::string& targetWindowName);
    void update();

    // --- Drawing API (operates on the back buffer) ---
    void clearBackBuffer();
    void drawLine(int x1, int y1, int x2, int y2, const Color& color, int thickness = 1);
    void drawBox(int x, int y, int width, int height, const Color& color, int thickness = 1);
    void drawText(int x, int y, const std::string& text, const Color& color);

    void swapBuffers();

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    Display* dpy;
    Window targetWindow;
    Window overlayWindow;
    GC gc;
    Pixmap backBuffer;
    int width, height;
    bool initialized;

    Window findWindowByName(const std::string& name);
    unsigned long allocateColor(const Color& color);
};

#endif //OVERLAY_H