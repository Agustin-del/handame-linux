#include <cstring>
#include <xcb/xcb.h>

int main() {
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_window_t window = xcb_generate_id(conn);
  xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0,
                    screen->width_in_pixels, screen->height_in_pixels, 10,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0,
                    NULL);
  const char *title = "handmade hero";

  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,XCB_ATOM_STRING,8, strlen(title), title);
  xcb_map_window(conn, window);
  xcb_flush(conn);
  return (0);
}
