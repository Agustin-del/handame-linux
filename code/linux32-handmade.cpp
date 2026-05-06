#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

int getEvents(xcb_connection_t *conn, xcb_window_t window, xcb_screen_t *screen,
              xcb_atom_t ct, uint32_t md) {
  xcb_generic_event_t *event = xcb_wait_for_event(conn);
  switch (event->response_type & ~0x80) {
  case XCB_FOCUS_IN: {
  } break;
  case XCB_FOCUS_OUT: {
  } break;
  case XCB_EXPOSE: {
    xcb_expose_event_t *ee = (xcb_expose_event_t *)event;
    if (ee->count == 0) {
      xcb_gcontext_t ctx = xcb_generate_id(conn);
      static uint32_t color = screen->white_pixel;
      if (color == screen->white_pixel) {
        color = screen->black_pixel;
      } else {
        color = screen->white_pixel;
      }
      xcb_create_gc(conn, ctx, window, XCB_GC_FOREGROUND, &color);
      xcb_rectangle_t rec = {0, 0, screen->height_in_pixels,
                             screen->width_in_pixels};
      xcb_poly_fill_rectangle(conn, window, ctx, 1, &rec);
      xcb_flush(conn);
    }
  } break;
  case XCB_CONFIGURE_NOTIFY: {
  } break;
  case XCB_DESTROY_NOTIFY: {
  }
  case XCB_CLIENT_MESSAGE: {
    xcb_client_message_event_t *cm = (xcb_client_message_event_t *)event;
    if (cm->type == ct && cm->data.data32[0] == md) {
      return 0;
    }
  }
  default: {

  } break;
  }
  free(event);
  return 1;
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

  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, wm_protocols, 4, 32,
                      1, &wm_delete_window);

  const char *title = "handmade hero";

  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
                      XCB_ATOM_STRING, 8, strlen(title), title);

  xcb_map_window(conn, window);
  xcb_flush(conn);

  for (;;) {
    int event_result =
        getEvents(conn, window, screen, wm_protocols, wm_delete_window);
    if (!event_result) {
      break;
    }
  }

  xcb_disconnect(conn);
  return (0);
}
