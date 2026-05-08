#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable bool running;
global_variable void *bitMapMemory;
global_variable xcb_gcontext_t gContext;
global_variable uint16_t backBufferWidth;
global_variable uint16_t backBufferHeight;

internal void xUpdateWindow(xcb_connection_t *conn, xcb_window_t window,
                            xcb_screen_t *screen, uint16_t width,
                            uint16_t height) {
  uint32_t data_len = width * height * 4;
  xcb_put_image(conn, XCB_IMAGE_FORMAT_Z_PIXMAP, window, gContext, width,
                height, 0, 0, 0, screen->root_depth, data_len,
                (const uint8_t *)bitMapMemory);

  xcb_flush(conn);
}

internal void xResizeBackBuffer(uint16_t width, uint16_t height) {
  if (bitMapMemory) {
    free(bitMapMemory);
  }

  backBufferWidth = width;
  backBufferHeight = height;

  bitMapMemory = malloc(backBufferWidth * backBufferHeight * 4);
  if (!bitMapMemory) {
    printf("error alocando el backbuffer");
    return;
  }
#if 0
  uint32_t *pixel = (uint32_t *)bitMapMemory;
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      *pixel++ = 0x00FF00FF;
    }
  }

#endif
}

void xGetEvents(xcb_connection_t *conn, xcb_window_t window,
                xcb_screen_t *screen, xcb_atom_t ct, uint32_t md) {
  xcb_generic_event_t *event = xcb_wait_for_event(conn);
  switch (event->response_type & ~0x80) {
  case XCB_FOCUS_IN: {
  } break;
  case XCB_FOCUS_OUT: {
  } break;
  case XCB_EXPOSE: {
    xcb_expose_event_t *ee = (xcb_expose_event_t *)event;
    if (ee->count == 0) {
      xUpdateWindow(conn, ee->window, screen, backBufferWidth, backBufferHeight);
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

  xcb_intern_atom_cookie_t wm_protocols_cookie =
      xcb_intern_atom(conn, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
  xcb_intern_atom_cookie_t wm_delete_cookie =
      xcb_intern_atom(conn, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");

  xcb_intern_atom_reply_t *wm_protocols_reply =
      xcb_intern_atom_reply(conn, wm_protocols_cookie, NULL);
  xcb_intern_atom_reply_t *wm_delete_reply =
      xcb_intern_atom_reply(conn, wm_delete_cookie, NULL);

  xcb_atom_t wm_protocols = wm_protocols_reply->atom;
  xcb_atom_t wm_delete_window = wm_delete_reply->atom;

  free(wm_protocols_reply);
  free(wm_delete_reply);

  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, wm_protocols, 4, 32,
                      1, &wm_delete_window);

  const char *title = "handmade hero";

  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
                      XCB_ATOM_STRING, 8, strlen(title), title);

  gContext = xcb_generate_id(conn);
  uint32_t values = screen->black_pixel;
  xcb_create_gc(conn, gContext, window, XCB_GC_FOREGROUND, &values);

  xcb_map_window(conn, window);
  xcb_flush(conn);

  running = true;
  while (running) {
    xGetEvents(conn, window, screen, wm_protocols, wm_delete_window);
  }

  xcb_disconnect(conn);
  return (0);
}
