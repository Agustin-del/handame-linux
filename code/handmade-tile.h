#ifndef HANDMADE_TILE_H
struct tile_map_position{

  uint32 absTileX;
  uint32 absTileY;

  real32 tileRelX;
  real32 tileRelY;
};

struct tile_chunk_position {
  uint32 tileChunkX;
  uint32 tileChunkY;

  uint32 relTileX;
  uint32 relTileY;
};

struct tile_chunk {
  uint32 *tiles;
};

struct tile_map {

  uint32 chunkShift;
  uint32 chunkMask;
  uint32 chunkDim;

  real32 tileSideInMeters;
  int32 tileSideInPixels;
  real32 metersToPixels;

  uint32 tileChunkCountX;
  uint32 tileChunkCountY;

  tile_chunk *tileChunks;
};

#define HANDMADE_TILE_H
#endif
