#include "Overlay.h"
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <iostream>
#include <cstring>
#include <stdexcept>

// --- Static Color Definitions ---
// These are defined here in the implementation file.
const Overlay::Color Overlay::RED     = {255, 0, 0};
const Overlay::Color Overlay::GREEN   = {0, 255, 0};
const Overlay::Color Overlay::BLUE    = {0, 0, 255};
const Overlay::Color Overlay::WHITE   = {255, 255, 255};
const Overlay::Color Overlay::BLACK   = {0, 0, 0};
const Overlay::Color Overlay::YELLOW  = {255, 255, 0};
const Overlay::Color Overlay::CYAN    = {0, 255, 255};
const Overlay::Color Overlay::MAGENTA = {255, 0, 255};


Overlay::Overlay()
    : dpy(nullptr), targetWindow(0), overlayWindow(0), gc(nullptr),
      backBuffer(0), width(0), height(0), initialized(false) {}

Overlay::~Overlay() {
    if (!initialized) {
        return;
    }
    if (backBuffer) XFreePixmap(dpy, backBuffer);
    if (gc) XFreeGC(dpy, gc);
    if (overlayWindow) XDestroyWindow(dpy, overlayWindow);
    if (dpy) XCloseDisplay(dpy);
}

bool Overlay::initialize(const std::string& targetWindowName) {
    dpy = XOpenDisplay(nullptr);
    if (!dpy) {
        std::cerr << "Error: Could not open X display." << std::endl;
        return false;
    }

    targetWindow = findWindowByName(targetWindowName);
    if (targetWindow == 0) {
        std::cerr << "Error: Could not find window with name: " << targetWindowName << std::endl;
        XCloseDisplay(dpy);
        dpy = nullptr;
        return false;
    }

    XWindowAttributes target_attrs;
    XGetWindowAttributes(dpy, targetWindow, &target_attrs);
    width = target_attrs.width;
    height = target_attrs.height;

    XVisualInfo vinfo;
    if (!XMatchVisualInfo(dpy, DefaultScreen(dpy), 32, TrueColor, &vinfo)) {
        std::cerr << "Error: No 32-bit visual found. Cannot create transparent window." << std::endl;
        XCloseDisplay(dpy);
        dpy = nullptr;
        return false;
    }

    Colormap colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), vinfo.visual, AllocNone);
    XSetWindowAttributes attrs;
    attrs.colormap = colormap;
    attrs.border_pixel = 0;
    attrs.background_pixel = 0;
    attrs.override_redirect = True;

    overlayWindow = XCreateWindow(
        dpy, DefaultRootWindow(dpy),
        target_attrs.x, target_attrs.y,
        width, height, 0, vinfo.depth, InputOutput, vinfo.visual,
        CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect,
        &attrs
    );

    XserverRegion region = XFixesCreateRegion(dpy, nullptr, 0);
    XFixesSetWindowShapeRegion(dpy, overlayWindow, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(dpy, region);

    XMapWindow(dpy, overlayWindow);
    backBuffer = XCreatePixmap(dpy, overlayWindow, width, height, vinfo.depth);
    if (!backBuffer) {
        std::cerr << "Error: Could not create back buffer pixmap." << std::endl;
        return false;
    }

    gc = XCreateGC(dpy, backBuffer, 0, nullptr);
    XFlush(dpy);
    initialized = true;
    std::cout << "Overlay initialized successfully on top of '" << targetWindowName << "'." << std::endl;
    return true;
}

void Overlay::update() {
    if (!initialized) return;

    XWindowAttributes target_attrs;
    if (XGetWindowAttributes(dpy, targetWindow, &target_attrs) == 0) {
        std::cerr << "Warning: Could not get attributes of target window." << std::endl;
        return;
    }

    XWindowAttributes overlay_attrs;
    XGetWindowAttributes(dpy, overlayWindow, &overlay_attrs);

    bool needs_resize = (target_attrs.width != width || target_attrs.height != height);
    bool needs_move = (target_attrs.x != overlay_attrs.x || target_attrs.y != overlay_attrs.y);

    if (needs_move || needs_resize) {
        XMoveResizeWindow(dpy, overlayWindow, target_attrs.x, target_attrs.y, target_attrs.width, target_attrs.height);

        if (needs_resize) {
            width = target_attrs.width;
            height = target_attrs.height;
            XFreePixmap(dpy, backBuffer);
            XVisualInfo vinfo;
            XMatchVisualInfo(dpy, DefaultScreen(dpy), 32, TrueColor, &vinfo);
            backBuffer = XCreatePixmap(dpy, overlayWindow, width, height, vinfo.depth);
        }
    }
    XRaiseWindow(dpy, overlayWindow);
    XFlush(dpy);
}

void Overlay::clearBackBuffer() {
    if (!initialized) return;
    XSetForeground(dpy, gc, 0);
    XFillRectangle(dpy, backBuffer, gc, 0, 0, width, height);
}

void Overlay::swapBuffers() {
    if (!initialized) return;
    XCopyArea(dpy, backBuffer, overlayWindow, gc, 0, 0, width, height, 0, 0);
    XFlush(dpy);
}

void Overlay::drawLine(int x1, int y1, int x2, int y2, const Color& color, int thickness) {
    if (!initialized) return;
    XSetForeground(dpy, gc, allocateColor(color));
    XSetLineAttributes(dpy, gc, thickness, LineSolid, CapButt, JoinMiter);
    XDrawLine(dpy, backBuffer, gc, x1, y1, x2, y2);
}

void Overlay::drawBox(int x, int y, int w, int h, const Color& color, int thickness) {
    if (!initialized) return;
    drawLine(x, y, x + w, y, color, thickness);
    drawLine(x + w, y, x + w, y + h, color, thickness);
    drawLine(x + w, y + h, x, y + h, color, thickness);
    drawLine(x, y + h, x, y, color, thickness);
}

void Overlay::drawText(int x, int y, const std::string& text, const Color& color) {
    if (!initialized) return;
    XSetForeground(dpy, gc, allocateColor(color));
    XDrawString(dpy, backBuffer, gc, x, y, text.c_str(), text.length());
}

Window Overlay::findWindowByName(const std::string& name) {
    Window root = DefaultRootWindow(dpy);
    Window parent, *children;
    unsigned int n_children;

    if (XQueryTree(dpy, root, &root, &parent, &children, &n_children) == 0) {
        return 0;
    }

    Window found_window = 0;
    for (unsigned int i = 0; i < n_children; ++i) {
        char* window_name = nullptr;
        if (XFetchName(dpy, children[i], &window_name) > 0) {
            if (window_name != nullptr && std::string(window_name) == name) {
                found_window = children[i];
                XFree(window_name);
                break;
            }
            if (window_name) XFree(window_name);
        }
    }

    if (children) XFree(children);
    return found_window;
}

unsigned long Overlay::allocateColor(const Color& color) {
    return (0xFF << 24) | (color.r << 16) | (color.g << 8) | color.b;
}