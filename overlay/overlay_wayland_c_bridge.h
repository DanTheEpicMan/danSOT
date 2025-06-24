#ifndef OVERLAY_WAYLAND_C_BRIDGE_H
#define OVERLAY_WAYLAND_C_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    // Opaque struct. C++ code will only ever see a pointer to this.
    struct wlc_context;

    struct wlc_buffer {
        void* data;
        int width;
        int height;
        int stride;
    };

    // --- Public API for the C Bridge ---

    // No longer needs target_app_id
    struct wlc_context* wlc_create(void);
    void wlc_destroy(struct wlc_context* ctx);

    // Dispatch Wayland events
    int wlc_dispatch(struct wlc_context* ctx);

    // Get the current dimensions of the overlay surface.
    // Returns false if not yet configured.
    bool wlc_get_dimensions(struct wlc_context* ctx, int* width, int* height);

    // Drawing buffer management
    struct wlc_buffer* wlc_get_draw_buffer(struct wlc_context* ctx);
    void wlc_swap_buffers(struct wlc_context* ctx);

#ifdef __cplusplus
}
#endif

#endif // OVERLAY_WAYLAND_C_BRIDGE_H