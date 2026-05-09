#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

global_variable bool running;
global_variable void *bitMapMemory;
global_variable xcb_gcontext_t gContext;
global_variable uint16 bitMapWidth;
global_variable uint16 bitMapHeight;
global_variable int bytesPerPixel = 4;

internal void renderWeirdGradient(int xOffset, int yOffset) {
  int pitch = bitMapWidth * bytesPerPixel;
  uint8 *row = (uint8 *)bitMapMemory;
  for (int y = 0; y < bitMapHeight; ++y) {
    uint32 *pixel = (uint32 *)row;
    for (int x = 0; x < bitMapWidth; ++x) {
      uint8 blue = x + xOffset;
      uint8 green = y + yOffset;
      *pixel++ = (green << 8) | blue; 
    }
    row += pitch;
  }
}

internal void xResizeBackBuffer(uint16 width, uint16 height) {
  if (bitMapMemory) {
    free(bitMapMemory);
  }
  bitMapWidth = width;
  bitMapHeight = height;
  uint32 bitMapMemorySize = width * height * bytesPerPixel;
  // TODO:Usar mmap y munmap
  bitMapMemory = malloc(bitMapMemorySize);
  if (!bitMapMemory) {
    printf("error alocando el backbuffer");
    return;
  }
}

internal void xUpdateWindow(xcb_connection_t *conn, xcb_window_t window,
                            xcb_screen_t *screen) {
  uint32 bitMapMemorySize = bitMapWidth * bitMapHeight * bytesPerPixel;

  xcb_put_image(conn, XCB_IMAGE_FORMAT_Z_PIXMAP, window, gContext, bitMapWidth,
                bitMapHeight, 0, 0, 0, screen->root_depth, bitMapMemorySize,
                (uint8 *)bitMapMemory);
  xcb_flush(conn);
}

void xGetEvents(xcb_connection_t *conn, xcb_window_t window,
                xcb_screen_t *screen, xcb_atom_t ct, uint32 md) {

  xcb_generic_event_t *event = xcb_poll_for_event(conn);
  if (event) {
    switch (event->response_type & ~0x80) {
    case XCB_FOCUS_IN: {
    } break;
    case XCB_FOCUS_OUT: {
    } break;
    case XCB_EXPOSE: {
      xcb_expose_event_t *ee = (xcb_expose_event_t *)event;
      if (ee->count == 0) {
        xUpdateWindow(conn, ee->window, screen);
      }
    } break;
    case XCB_CONFIGURE_NOTIFY: {
      xcb_configure_notify_event_t *cn = (xcb_configure_notify_event_t *)event;
      xResizeBackBuffer(cn->width, cn->height);
    } break;
    case XCB_CLIENT_MESSAGE: {
      xcb_client_message_event_t *cm = (xcb_client_message_event_t *)event;
      if (cm->type == ct && cm->data.data32[0] == md) {
        running = false;
      }
    } break;
    default: {

    } break;
    }
    free(event);
  }
}

int main() {
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  if (xcb_connection_has_error(conn)) {
    return 1;
  }

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_window_t window = xcb_generate_id(conn);
  xcb_event_mask_t events =
      (xcb_event_mask_t)(XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_EXPOSURE |
                         XCB_EVENT_MASK_STRUCTURE_NOTIFY);
  xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0,
                    screen->width_in_pixels, screen->height_in_pixels, 10,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                    XCB_CW_EVENT_MASK, &events);

  xcb_intern_atom_cookie_t wmProtocolsCookie =
      xcb_intern_atom(conn, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
  xcb_intern_atom_cookie_t wmDeleteCookie =
      xcb_intern_atom(conn, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");

  xcb_intern_atom_reply_t *wmProtocolsReply =
      xcb_intern_atom_reply(conn, wmProtocolsCookie, NULL);
  xcb_intern_atom_reply_t *wmDeleteReply =
      xcb_intern_atom_reply(conn, wmDeleteCookie, NULL);

  xcb_atom_t wmProtocols = wmProtocolsReply->atom;
  xcb_atom_t wmDeleteWindow = wmDeleteReply->atom;

  free(wmProtocolsReply);
  free(wmDeleteReply);

  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, wmProtocols, 4, 32,
                      1, &wmDeleteWindow);

  const char *title = "handmade hero";

  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
                      XCB_ATOM_STRING, 8, strlen(title), title);

  gContext = xcb_generate_id(conn);
  uint32 values = screen->black_pixel;
  xcb_create_gc(conn, gContext, window, XCB_GC_FOREGROUND, &values);

  xcb_map_window(conn, window);
  xcb_flush(conn);

  running = true;
  int xOffset = 0;
  int yOffset = 0;
  while (running) {
    xGetEvents(conn, window, screen, wmProtocols, wmDeleteWindow);
    renderWeirdGradient(xOffset, yOffset);
    xUpdateWindow(conn, window, screen);
    xOffset++;
  }

  xcb_disconnect(conn);
  return (0);
}
