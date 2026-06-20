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

struct memory_arena {

  memory_index size;
  uint8 *base;
  memory_index used;
};

internal void initializeArena(memory_arena *arena, memory_index size,
                              uint8 *base) {
  arena->size = size;
  arena->base = base;
  arena->used = 0;
}

#define pushStruct(arena, type) (type *)pushSize_(arena, sizeof(type))
#define pushArray(arena, count, type)                                          \
  (type *)pushSize_(arena, (count) * sizeof(type))
void *pushSize_(memory_arena *arena, memory_index size) {
  assert(arena->used + size <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;

  return result;
}

#include "handmade-intrinsics.h"
#include "handmade-tile.h"

struct world {
  tile_map *tileMap;
};

struct loaded_bitmap {
  int32 width;
  int32 height;
  uint32 *pixels;
};

struct game_state {
  memory_arena worldArena;
  world *world;
  tile_map_position playerP;
  loaded_bitmap backdrop;
  loaded_bitmap heroHead;
  loaded_bitmap heroCape;
  loaded_bitmap heroTorso;
};

#define HANDMADE_H
#endif
