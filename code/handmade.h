#ifndef HANDMADE_H
#define HANDMADE_H
  struct game_offscreen_buffer {
    void *memory;
    uint16 width;
    uint16 height;
    int bytesPerPixel;
    int pitch;
  };

  struct game_sound_output_buffer {
    int samplesPerSecond;
    int sampleCount;
    int16 *samples;
  };

  internal void gameOutputSound(game_sound_output_buffer *soundBuffer, int toneHz);
  internal void gameUpdateAndRender(game_offscreen_buffer *buffer, int blueOffset, int greenOffset, game_sound_output_buffer *soundBuffer, int toneHz);
#endif
