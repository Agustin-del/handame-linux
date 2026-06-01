#ifndef HANDMADE_H
#include <math.h>
#include <stdint.h>
#define internal static
#define local_persist static
#define global_variable static

#define PI32 3.14159265359f
#define NS 1000000000ULL

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

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

struct thread_context {
  int placeholder;
};

#if HANDMADE_INTERNAL
struct debug_read_file_result {
  void *contents;
  uint32 contentsSize;
};

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name)                                  \
  debug_read_file_result name(thread_context *thread, char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name)                                 \
  bool32 name(thread_context *thread, char *filename, uint32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name)                                  \
  void name(thread_context *thread, debug_read_file_result *file)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

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
  game_button_state mouseButtons[2];
  int32 mouseX, mouseY, mouseZ;
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

  debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;

  debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;

  debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
};


#define GAME_UPDATE_AND_RENDER(name)                                           \
  void name(thread_context *thread, game_memory *memory, game_input *input,                            \
            game_offscreen_buffer *buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name)                                           \
  void name(thread_context *thread, game_memory *memory, game_sound_output_buffer *soundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

struct game_state {
  int toneHz;
  int greenOffset;
  int blueOffset;
  real32 tSine;

  int playerX;
  int playerY;

  real32 tJump;
};

#define HANDMADE_H
#endif
