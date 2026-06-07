inline void recanonicalizeCoord(tile_map *tileMap, uint32 *tile,
                                real32 *tileRel) {

  int32 offset = roundReal32ToInt32(*tileRel / tileMap->tileSideInMeters);
  *tile += offset;
  *tileRel -= offset * tileMap->tileSideInMeters;

  assert(*tileRel >= -0.5f * tileMap->tileSideInMeters);
  assert(*tileRel <= 0.5f * tileMap->tileSideInMeters);
}

inline tile_map_position recanonicalizePosition(tile_map *tileMap,
                                                tile_map_position pos) {

  tile_map_position result = pos;

  recanonicalizeCoord(tileMap, &result.absTileX, &result.tileRelX);
  recanonicalizeCoord(tileMap, &result.absTileY, &result.tileRelY);

  return result;
}

inline tile_chunk *getTileChunk(tile_map *tileMap, uint32 tileChunkX,
                                uint32 tileChunkY) {
  tile_chunk *tileChunk = {};
  if ((tileChunkX >= 0) && (tileChunkX < tileMap->tileChunkCountX) &&
      (tileChunkY >= 0) && (tileChunkY < tileMap->tileChunkCountY)) {
    tileChunk =
        &tileMap
             ->tileChunks[tileMap->tileChunkCountX * tileChunkY + tileChunkX];
  }
  return tileChunk;
}

inline uint32 getTileValueUnchecked(tile_map *tileMap, tile_chunk *tileChunk,
                                    uint32 tileX, uint32 tileY) {
  assert(tileChunk);
  assert(tileX < tileMap->chunkDim);
  assert(tileY < tileMap->chunkDim);

  uint32 tileChunkValue = tileChunk->tiles[tileMap->chunkDim * tileY + tileX];
  return tileChunkValue;
}
inline void setTileValueUnchecked(tile_map *tileMap, tile_chunk *tileChunk,
                                  uint32 tileX, uint32 tileY,
                                  uint32 tileValue){

  assert(tileChunk);
  assert(tileX < tileMap->chunkDim);
  assert(tileY < tileMap->chunkDim);
  tileChunk->tiles[tileMap->chunkDim * tileY + tileX] = tileValue;
}


internal uint32 getTileValue(tile_map *tileMap, tile_chunk *tileChunk,
                             uint32 testTileX, uint32 testTileY) {

  uint32 tileChunkValue = 0;
  if (tileChunk) {
    tileChunkValue =
        getTileValueUnchecked(tileMap, tileChunk, testTileX, testTileY);
  }

  return tileChunkValue;
}

internal void setTileValue(tile_map *tileMap, tile_chunk *tileChunk, uint32 testTileX,
                           uint32 testTileY, uint32 tileValue) {

  if (tileChunk) {
    setTileValueUnchecked(tileMap, tileChunk, testTileX, testTileY, tileValue);
  }
}

inline tile_chunk_position
getChunkPositionFor(tile_map *tileMap, uint32 absTileX, uint32 absTileY) {
  tile_chunk_position result;

  result.tileChunkX = absTileX >> tileMap->chunkShift;
  result.tileChunkY = absTileY >> tileMap->chunkShift;
  result.relTileX = absTileX & tileMap->chunkMask;
  result.relTileY = absTileY & tileMap->chunkMask;

  return result;
}

internal uint32 getTileValue(tile_map *tileMap, uint32 absTileX,
                             uint32 absTileY) {
  bool32 empty = false;

  tile_chunk_position chunkPos =
      getChunkPositionFor(tileMap, absTileX, absTileY);
  tile_chunk *tileChunk =
      getTileChunk(tileMap, chunkPos.tileChunkX, chunkPos.tileChunkY);
  uint32 tileChunkValue =
      getTileValue(tileMap, tileChunk, chunkPos.relTileX, chunkPos.relTileY);
  return tileChunkValue;
}

internal bool32 isTileMapPointEmpty(tile_map *tileMap,
                                    tile_map_position canPos) {
  uint32 tileChunkValue =
      getTileValue(tileMap, canPos.absTileX, canPos.absTileY);
  bool32 empty = (tileChunkValue == 0);
  return empty;
}

internal void setTileValue(memory_arena *arena, tile_map *tileMap,
                           uint32 absTileX, uint32 absTileY, uint32 tileValue) {
  tile_chunk_position chunkPos =
      getChunkPositionFor(tileMap, absTileX, absTileY);

  tile_chunk *tileChunk =
      getTileChunk(tileMap, chunkPos.tileChunkX, chunkPos.tileChunkY);

  assert(tileChunk);

  setTileValue(tileMap, tileChunk, chunkPos.relTileX, chunkPos.relTileY,
               tileValue);
}
