#ifndef LINUX32_HANDMADE_H

struct linux32_offscreen_buffer {
  void *memory;
  uint16 width;
  uint16 height;
  int bytesPerPixel;
  int pitch;
};

struct linux32_sound_output {
  int framesPerSecond;
  uint32 bytesPerFrame;
  int latencyFramesCount;
};

#define LINUX32_HANDMADE_H
#endif
