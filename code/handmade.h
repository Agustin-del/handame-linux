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

struct canonical_position{
  int32 tilemapX;
  int32 tilemapY;

  int32 tileX;
  int32 tileY;

  real32 tileRelativeX;
  real32 tileRelativeY;
};

struct raw_position{
  int32 tilemapX;
  int32 tilemapY;

  real32 x;
  real32 y;
};

struct tile_map {
  uint32 *tiles;
};

struct world_map {
  tile_map *tilemaps;

  int32 tilemapCountX;
  int32 tilemapCountY;

  int32 countX;
  int32 countY;

  real32 upperLeftX;
  real32 upperLeftY;
  real32 tileWidth;
  real32 tileHeight;
};

struct game_state {
  int32 playerTilemapX;
  int32 playerTilemapY;
  real32 playerX;
  real32 playerY;
};

#define HANDMADE_H
#endif
