#include "overlay_wayland_c_bridge.h"

#define OVERLAY_DEBUG 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

#if OVERLAY_DEBUG
#define D_PRINT(fmt, ...) do { fprintf(stdout, "[WLC_DEBUG] " fmt "\n", ##__VA_ARGS__); fflush(stdout); } while(0)
#else
#define D_PRINT(fmt, ...) do {} while(0)
#endif

struct wlc_internal_buffer {
    struct wl_buffer* wl_buffer;
    void* data;
    int size;
};

struct wlc_context {
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct zwlr_layer_shell_v1* layer_shell;
    struct wl_surface* surface;
    struct zwlr_layer_surface_v1* layer_surface;
    struct wl_output* target_output;
    int width, height, stride;
    bool configured;
    struct wlc_internal_buffer buffers[2];
    int current_buffer_idx;
    struct wlc_buffer public_buffer;
};

static int create_shm_file() {
    char name[] = "/wl_shm-XXXXXX";
    int fd = mktemp(name) ? shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600) : -1;
    if (fd >= 0) { shm_unlink(name); return fd; }
    return -1;
}

static void release_buffer(struct wlc_internal_buffer* buffer) {
    if (!buffer) return;
    if (buffer->wl_buffer) wl_buffer_destroy(buffer->wl_buffer);
    if (buffer->data) munmap(buffer->data, buffer->size);
    memset(buffer, 0, sizeof(struct wlc_internal_buffer));
}

static bool create_buffer(struct wlc_context* ctx, int idx, int w, int h) {
    int stride = w * 4;
    int size = stride * h;
    int fd = create_shm_file();
    if (fd < 0 || ftruncate(fd, size) < 0) { if (fd >= 0) close(fd); return false; }
    void* data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) { close(fd); return false; }
    struct wl_shm_pool* pool = wl_shm_create_pool(ctx->shm, fd, size);
    ctx->buffers[idx].wl_buffer = wl_shm_pool_create_buffer(pool, 0, w, h, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
    ctx->buffers[idx].data = data;
    ctx->buffers[idx].size = size;
    return true;
}

static void layer_surface_configure(void* data, struct zwlr_layer_surface_v1* s, uint32_t serial, uint32_t w, uint32_t h) {
    struct wlc_context* ctx = data;
    D_PRINT("Received layer_surface.configure event! width=%u, height=%u", w, h);
    if (ctx->width != w || ctx->height != h) {
        D_PRINT("Dimensions changed. Recreating buffers.");
        ctx->width = w; ctx->height = h; ctx->stride = w * 4;
        release_buffer(&ctx->buffers[0]); release_buffer(&ctx->buffers[1]);
        create_buffer(ctx, 0, w, h); create_buffer(ctx, 1, w, h);
    }
    ctx->configured = true;
    zwlr_layer_surface_v1_ack_configure(s, serial);
}
static void layer_surface_closed(void* data, struct zwlr_layer_surface_v1* s) {
    D_PRINT("Layer surface closed by compositor.");
    ((struct wlc_context*)data)->configured = false;
}
static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {.configure = layer_surface_configure, .closed = layer_surface_closed};

static void registry_global(void* data, struct wl_registry* r, uint32_t name, const char* i, uint32_t v) {
    struct wlc_context* ctx = data;
    if (strcmp(i, wl_compositor_interface.name)==0) ctx->compositor = wl_registry_bind(r, name, &wl_compositor_interface, 4);
    else if (strcmp(i, wl_shm_interface.name)==0) ctx->shm = wl_registry_bind(r, name, &wl_shm_interface, 1);
    else if (strcmp(i, zwlr_layer_shell_v1_interface.name)==0) ctx->layer_shell = wl_registry_bind(r, name, &zwlr_layer_shell_v1_interface, 1);
    else if (strcmp(i, wl_output_interface.name)==0) if (!ctx->target_output) ctx->target_output = wl_registry_bind(r, name, &wl_output_interface, 1);
}
static const struct wl_registry_listener registry_listener = {.global = registry_global, .global_remove = (void*)registry_global};

struct wlc_context* wlc_create(void) {
    D_PRINT("--- Starting Wayland Overlay Initialization (Simple Mode) ---");
    struct wlc_context* ctx = calloc(1, sizeof(struct wlc_context));
    if (!ctx) return NULL;

    ctx->display = wl_display_connect(NULL);
    if (!ctx->display) { D_PRINT("ERROR: wl_display_connect failed."); wlc_destroy(ctx); return NULL; }
    D_PRINT("Connected to Wayland display.");

    ctx->registry = wl_display_get_registry(ctx->display);
    wl_registry_add_listener(ctx->registry, &registry_listener, ctx);
    D_PRINT("Dispatching once to get globals...");
    wl_display_roundtrip(ctx->display);

    if (!ctx->compositor || !ctx->shm || !ctx->layer_shell) {
        D_PRINT("ERROR: A required Wayland protocol was not found.");
        wlc_destroy(ctx); return NULL;
    }
    D_PRINT("All required Wayland protocols found.");

    if (!ctx->target_output) { D_PRINT("ERROR: No wl_output found."); wlc_destroy(ctx); return NULL; }
    D_PRINT("Using first available wl_output for the overlay.");

    ctx->surface = wl_compositor_create_surface(ctx->compositor);
    ctx->layer_surface = zwlr_layer_shell_v1_get_layer_surface(ctx->layer_shell, ctx->surface, ctx->target_output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "overlay");
    D_PRINT("Created layer surface.");

    // Set both input and opaque regions to empty for full transparency.
    struct wl_region *region = wl_compositor_create_region(ctx->compositor);
    wl_surface_set_input_region(ctx->surface, region);
    wl_surface_set_opaque_region(ctx->surface, region);
    wl_region_destroy(region);
    D_PRINT("Set input and opaque regions to NULL for transparency and click-through.");

    zwlr_layer_surface_v1_set_size(ctx->layer_surface, 0, 0);
    zwlr_layer_surface_v1_set_anchor(ctx->layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(ctx->layer_surface, -1);
    zwlr_layer_surface_v1_set_keyboard_interactivity(ctx->layer_surface, 0);
    zwlr_layer_surface_v1_add_listener(ctx->layer_surface, &layer_surface_listener, ctx);

    wl_surface_commit(ctx->surface);
    D_PRINT("Committed initial surface state. Now waiting for configure event...");
    return ctx;
}
void wlc_destroy(struct wlc_context* ctx) {
    if (!ctx) return;
    D_PRINT("--- Destroying Wayland Overlay ---");
    release_buffer(&ctx->buffers[0]); release_buffer(&ctx->buffers[1]);
    if (ctx->layer_surface) zwlr_layer_surface_v1_destroy(ctx->layer_surface);
    if (ctx->surface) wl_surface_destroy(ctx->surface);
    if (ctx->display) wl_display_disconnect(ctx->display);
    free(ctx);
}
int wlc_dispatch(struct wlc_context* ctx) { return wl_display_dispatch(ctx->display); }
bool wlc_get_dimensions(struct wlc_context* ctx, int* w, int* h) { if (!ctx->configured) return false; *w = ctx->width; *h = ctx->height; return true; }
struct wlc_buffer* wlc_get_draw_buffer(struct wlc_context* ctx) {
    if (!ctx->configured) return NULL;
    ctx->current_buffer_idx = 1 - ctx->current_buffer_idx;
    struct wlc_internal_buffer* buf = &ctx->buffers[ctx->current_buffer_idx];
    ctx->public_buffer.data = buf->data; ctx->public_buffer.width = ctx->width;
    ctx->public_buffer.height = ctx->height; ctx->public_buffer.stride = ctx->stride;
    return &ctx->public_buffer;
}
void wlc_swap_buffers(struct wlc_context* ctx) {
    if (!ctx->configured) return;
    wl_surface_attach(ctx->surface, ctx->buffers[ctx->current_buffer_idx].wl_buffer, 0, 0);
    wl_surface_damage_buffer(ctx->surface, 0, 0, ctx->width, ctx->height);
    wl_surface_commit(ctx->surface);
}