#ifndef HANDMADE_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if __clang__
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#else 
#undef COMCOMPILER_MSVC
#define COMPILER_MSVC 1
#endif
#endif

typedef float real32;
typedef double real64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t memory_index;

typedef struct thread_context {
  int placeholder;
} thread_context;

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

typedef struct game_offscreen_buffer {
  void *memory;
  int bytesPerPixel;
  int pitch;
  uint16 width;
  uint16 height;
} game_offscreen_buffer;

typedef struct game_sound_output_buffer {
  int samplesPerSecond;
  int sampleCount;
  int16 *samples;
} game_sound_output_buffer;

typedef struct game_button_state {
  int halfTransitionCount;
  bool32 endedDown;
} game_button_state;

typedef struct game_controller_input {
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
} game_controller_input;

typedef struct game_input {
  game_button_state mouseButtons[3];
  int32 mouseX, mouseY;

  real32 dtForFrame;
  game_controller_input controllers[5];
}game_input;

typedef struct game_memory {
  uint64 permanentStorageSize;
  void *permanentStorage;
  uint64 transientStorageSize;
  void *transientStorage;
  bool32 isInitialized;

  debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;

  debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;

  debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
}game_memory;

#define GAME_UPDATE_AND_RENDER(name)                                           \
  void name(thread_context *thread, game_memory *memory, game_input *input,                            \
            game_offscreen_buffer *buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name)                                           \
  void name(thread_context *thread, game_memory *memory, game_sound_output_buffer *soundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);


#ifdef __cplusplus
}
#endif

#define HANDMADE_PLATFORM_H
#endif
