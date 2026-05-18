#ifndef HANDMADE_H
#define HANDMADE_H
  struct game_offscreen_buffer {
    void *memory;
    uint16 width;
    uint16 height;
    int bytesPerPixel;
    int pitch;
  };

  internal void gameUpdateAndRender(game_offscreen_buffer *buffer, int blueOffset, int greenOffset);
#endif
