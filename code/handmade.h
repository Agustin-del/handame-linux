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
internal bool32 DEBUGPlatformWriteEntireFile(char *filename, uint32 memorySize,
                                             void *memory);
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
    game_button_state buttons[12];
    struct {
      game_button_state moveUp;
      game_button_state moveDown;
      game_button_state moveLeft;
      game_button_state moveRight;

      game_button_state actionUp;
      game_button_state actionDown;
      game_button_state actionLeft;
      game_button_state actionRight;

      game_button_state leftShoulder;
      game_button_state rightShoulder;

      game_button_state back;
      game_button_state start;

      /**/
      game_button_state terminator;
    };
  };

  bool32 isConnected;
  bool32 isAnalog;
  real32 averageX;
  real32 averageY;
};

struct game_input {
  game_controller_input controllers[5];
};

inline game_controller_input *getController(game_input *input,
                                            int controllerIndex) {

  assert(controllerIndex < (int)arrayCount(input->controllers));

  game_controller_input *result = &input->controllers[controllerIndex];
  return result;
}

struct game_memory {
  uint64 permanentStorageSize;
  void *permanentStorage;
  uint64 transientStorageSize;
  void *transientStorage;
  bool32 isInitialized;
};

// Esto no es una llamada al juego, me parece.
internal void gameOutputSound(game_sound_output_buffer *soundBuffer,
                              int toneHz);

internal void gameUpdateAndRender(game_memory *memory, game_input *input,
                                  game_offscreen_buffer *buffer);

internal void getSoundSamples(game_memory *memory,
                              game_sound_output_buffer *soundBuffer);

struct game_state {
  int toneHz;
  int greenOffset;
  int blueOffset;
};

#define HANDMADE_H
#endif
