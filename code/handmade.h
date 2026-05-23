#ifndef HANDMADE_H
#if HANDMADE_SLOW
#define assert(expression)                                                     \
  if (!(expression)) {                                                         \
    *(int *)0 = 0;                                                             \
  }
#else
#define assert(expression)
#endif

#define kilobytes(value) (value * 1024LL)
#define megabytes(value) ((kilobytes(value)) * 1024LL)
#define gigabytes(value) ((megabytes(value)) * 1024LL)
#define terabytes(value) ((gigabytes(value)) * 1024LL)

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))

internal uint32 safeTruncateUint64(uint64 value) {
  assert((value < (uint64)0xffffffff));
  uint32 result = (uint32)value;
  return result;
}

#if HANDMADE_INTERNAL
struct debug_read_file_result {
  void *contents;
  uint32 contentsSize;
};

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *filename);
internal bool32 DEBUGPlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory);
internal void DEBUGPlatformFreeFileMemory(debug_read_file_result *file);
#endif

struct game_offscreen_buffer {
  void *memory;
  int bytesPerPixel;
  int pitch;
  uint16 width;
  uint16 height;
};

struct game_sound_output_buffer {
  int samplesPerSecond;
  int sampleCount;
  int16 *samples;
};

struct game_button_state {
  int halfTransitionCount;
  bool32 endedDown;
};

struct game_controller_input {
  union {
    game_button_state buttons[8];
    struct {
      game_button_state up;
      game_button_state down;
      game_button_state left;
      game_button_state right;
      game_button_state a;
      game_button_state s;
      game_button_state d;
      game_button_state w;
    };
  };

  bool32 isAnalog;
  real32 startX;
  real32 startY;

  real32 minX;
  real32 minY;

  real32 endX;
  real32 endY;
};

struct game_input {
  game_controller_input Controllers[4];
};

struct game_memory {
  uint64 permanentStorageSize;
  void *permanentStorage;
  uint64 transientStorageSize;
  void *transientStorage;
  bool32 isInitialized;
};

internal void gameOutputSound(game_sound_output_buffer *soundBuffer,
                              int toneHz);

internal void gameUpdateAndRender(game_memory *memory, game_input *input,
                                  game_offscreen_buffer *buffer,
                                  game_sound_output_buffer *soundBuffer);

struct game_state {
  int toneHz;
  int greenOffset;
  int blueOffset;
};

#define HANDMADE_H
#endif
