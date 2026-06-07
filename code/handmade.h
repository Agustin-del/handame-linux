#ifndef HANDMADE_H

#include "handmade-platform.h"

#define NS 1000000000ULL
#define PI32 3.14159265359f

#define internal static
#define local_persist static
#define global_variable static

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

inline game_controller_input *getController(game_input *input,
                                            int controllerIndex) {

  assert(controllerIndex < (int)arrayCount(input->controllers));

  game_controller_input *result = &input->controllers[controllerIndex];
  return result;
}

#include "handmade-intrinsics.h"
#include "handmade-tile.h"

struct memory_arena {

  memory_index size;
  uint8 *base;
  memory_index used;
};

struct world {
  tile_map *tileMap;
};

struct game_state {
  memory_arena worldArena;
  world *world;
  tile_map_position playerP;
};

#define HANDMADE_H
#endif
