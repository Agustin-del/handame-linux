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

struct tile_chunk_position {
  uint32 tileChunkX;
  uint32 tileChunkY;

  uint32 relTileX;
  uint32 relTileY;
};

struct world_position{
  uint32 absTileX;
  uint32 absTileY;

  real32 tileRelX;
  real32 tileRelY;
};

struct tile_chunk {
  uint32 *tiles;
};

struct world {
  tile_chunk *tileChunks;

  uint32 chunkShift;
  uint32 chunkMask;
  uint32 chunkDim;

  real32 tileSideInMeters;
  int32 tileSideInPixels;
  real32 metersToPixels;

  int32 tileChunkCountX;
  int32 tileChunkCountY;

};

struct game_state {
  world_position playerP;
};

#define HANDMADE_H
#endif
