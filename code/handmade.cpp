#include "handmade.h"
#include "handmade-intrinsics.h"
#include "handmade-platform.h"
#include <cstdio>

internal void drawRectangle(game_offscreen_buffer *buffer, real32 realMinX,
                            real32 realMinY, real32 realMaxX, real32 realMaxY,
                            real32 R, real32 G, real32 B) {

  int32 minX = roundReal32ToInt32(realMinX);
  int32 minY = roundReal32ToInt32(realMinY);
  int32 maxX = roundReal32ToInt32(realMaxX);
  int32 maxY = roundReal32ToInt32(realMaxY);

  if (minX < 0) {
    minX = 0;
  }
  if (minY < 0) {
    minY = 0;
  }

  if (maxX > buffer->width) {
    maxX = buffer->width;
  }

  if (maxY > buffer->height) {
    maxY = buffer->height;
  }

  uint32 color = (roundReal32ToUint32(R * 255.0f) << 16) |
                 roundReal32ToUint32(G * 255.0f) << 8 |
                 roundReal32ToUint32(B * 255.0f);

  uint8 *row = (uint8 *)buffer->memory + (buffer->pitch * minY) +
               buffer->bytesPerPixel * minX;
  for (int y = minY; y < maxY; ++y) {
    uint32 *pixel = (uint32 *)row;
    for (int x = minX; x < maxX; ++x) {
      *pixel++ = color;
    }
    row += buffer->pitch;
  }
}

internal void gameOutputSound(game_state *gameState,
                              game_sound_output_buffer *soundBuffer) {

  int16 toneVolume = 4000;
  int16 *sampleOut = soundBuffer->samples;

  for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount;
       ++sampleIndex) {

#if 0
    real32 sineValue = sinf(gameState->tSine);
    int16 sampleValue = (int16)(sineValue * toneVolume);
#else
    int16 sampleValue = 0;
#endif

    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;
#if 0

    gameState->tSine += 2.0f * PI32 / (real32)wavePeriod;
    if (gameState->tSine > 2.0f * PI32) {
      gameState->tSine -= 2.0f * PI32;
    }
#endif
  }
}

extern "C" GAME_GET_SOUND_SAMPLES(gameGetSoundSamples) {
  assert(sizeof(game_state) <= memory->permanentStorageSize);
  game_state *gameState = (game_state *)memory->permanentStorage;
  gameOutputSound(gameState, soundBuffer);
}

inline tile_chunk *getTileChunk(world *world, int32 tileChunkX,
                                int32 tileChunkY) {
  tile_chunk *tileChunk = {};
  if ((tileChunkX >= 0) && (tileChunkX < world->tileChunkCountX) &&
      (tileChunkY >= 0) && (tileChunkY < world->tileChunkCountY)) {
    tileChunk =
        &world->tileChunks[world->tileChunkCountX * tileChunkY + tileChunkX];
  }
  return tileChunk;
}

inline uint32 getTileValueUnchecked(world *world, tile_chunk *tileChunk,
                                    uint32 tileX, uint32 tileY) {
  assert(tileChunk);
  assert(tileX < world->chunkDim);
  assert(tileY < world->chunkDim);

  uint32 tileChunkValue = tileChunk->tiles[world->chunkDim * tileY + tileX];
  return tileChunkValue;
}

internal uint32 getTileValue(world *world, tile_chunk *tileChunk,
                             uint32 testTileX, uint32 testTileY) {

  uint32 tileChunkValue = 0;
  if (tileChunk) {
    tileChunkValue =
        getTileValueUnchecked(world, tileChunk, testTileX, testTileY);
  }

  return tileChunkValue;
}

inline void recanonicalizeCoord(world *world, uint32 *tile, real32 *tileRel) {

  int32 offset = floorReal32ToInt32(*tileRel / world->tileSideInMeters);
  *tile += offset;
  *tileRel -= offset * world->tileSideInMeters;

  assert(*tileRel >= 0);
  assert(*tileRel <= world->tileSideInMeters);
}

inline world_position recanonicalizePosition(world *world, world_position pos) {

  world_position result = pos;

  recanonicalizeCoord(world, &result.absTileX, &result.tileRelX);
  recanonicalizeCoord(world, &result.absTileY, &result.tileRelY);

  return result;
}

inline tile_chunk_position getChunkPosition(world *world, uint32 absTileX,
                                            uint32 absTileY) {
  tile_chunk_position result;

  result.tileChunkX = absTileX >> world->chunkShift;
  result.tileChunkY = absTileY >> world->chunkShift;
  result.relTileX = absTileX & world->chunkMask;
  result.relTileY = absTileY & world->chunkMask;

  return result;
}

internal uint32 getTileValue(world *world, uint32 absTileX, uint32 absTileY) {
  bool32 empty = false;

  tile_chunk_position chunkPos = getChunkPosition(world, absTileX, absTileY);
  tile_chunk *tileChunk =
      getTileChunk(world, chunkPos.tileChunkX, chunkPos.tileChunkY);
  uint32 tileChunkValue =
      getTileValue(world, tileChunk, chunkPos.relTileX, chunkPos.relTileY);
  return tileChunkValue;
}

internal bool32 isWorldPointEmpty(world *world, world_position canPos) {
  uint32 tileChunkValue = getTileValue(world, canPos.absTileX, canPos.absTileY);
  bool32 empty = (tileChunkValue == 0);
  return empty;
}

extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender) {

  assert((int)(&input->controllers[0].terminator -
               &input->controllers[0].buttons[0]) ==
         (int)(arrayCount(input->controllers[0].buttons)));

  assert(sizeof(game_state) <= memory->permanentStorageSize);

#define TILE_MAP_COUNT_X 256
#define TILE_MAP_COUNT_Y 256

  uint32 tempTiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
      {1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
       1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
      {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
       1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
       1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  };

  world world;
  world.chunkShift = 8;
  world.chunkMask = (1 << world.chunkShift) - 1;
  world.chunkDim = 256;

  world.tileChunkCountX = 1;
  world.tileChunkCountY = 1;

  tile_chunk tileChunks;
  tileChunks.tiles = (uint32 *)tempTiles;
  world.tileChunks = &tileChunks;

  world.tileSideInMeters = 1.4f;
  world.tileSideInPixels = 60;
  world.metersToPixels =
      (real32)world.tileSideInPixels / world.tileSideInMeters;

  real32 playerHeight = 1.4f;
  real32 playerWidth = 0.75f * playerHeight;

  game_state *gameState = (game_state *)memory->permanentStorage;

  if (!memory->isInitialized) {

    gameState->playerP.absTileX = 3;
    gameState->playerP.absTileY = 3;
    gameState->playerP.tileRelX = 5.0f;
    gameState->playerP.tileRelY = 5.0f;
    memory->isInitialized = true;
  }

  for (int controllerIdx = 0;
       controllerIdx < (int)arrayCount(input->controllers); ++controllerIdx) {
    game_controller_input *controller = getController(input, controllerIdx);
    if (controller->isAnalog) {
    } else {
      real32 dPlayerX = 0.0f;
      real32 dPlayerY = 0.0f;

      if (controller->moveUp.endedDown) {
        dPlayerY = 1.0f;
      }
      if (controller->moveDown.endedDown) {
        dPlayerY = -1.0f;
      }

      if (controller->moveLeft.endedDown) {
        dPlayerX = -1.0f;
      }

      if (controller->moveRight.endedDown) {
        dPlayerX = 1.0f;
      }

      dPlayerX *= 2.0f;
      dPlayerY *= 2.0f;

      world_position newPlayerP = gameState->playerP;

      newPlayerP.tileRelX += input->dtForFrame * dPlayerX;
      newPlayerP.tileRelY += input->dtForFrame * dPlayerY;

      newPlayerP = recanonicalizePosition(&world, newPlayerP);

      world_position playerLeft = newPlayerP;
      playerLeft.tileRelX -= (playerWidth * 0.5f);
      playerLeft = recanonicalizePosition(&world, playerLeft);

      world_position playerRight = newPlayerP;
      playerRight.tileRelX += (playerWidth * 0.5f);
      playerRight = recanonicalizePosition(&world, playerRight);

      if (isWorldPointEmpty(&world, newPlayerP) &&
          (isWorldPointEmpty(&world, playerLeft)) &&
          (isWorldPointEmpty(&world, playerRight))) {

        gameState->playerP = newPlayerP;
      }
    }
  }

  drawRectangle(buffer, 0.0f, 0.0f, buffer->width, buffer->height, 1.0f, 0.0f,
                1.0f);

  real32 centerX = (real32)buffer->width * 0.5f;
  real32 centerY = (real32)buffer->height * 0.5f;

  for (int32 relRow = -10; relRow < 10; ++relRow) {
    for (int32 relColumn = -20; relColumn < 20; ++relColumn) {

      uint32 column = relColumn + gameState->playerP.absTileX;
      uint32 row = relRow + gameState->playerP.absTileY;
      uint32 tileID = getTileValue(&world, column, row);
      real32 gray = 0.5f;

      if (tileID == 1) {
        gray = 1.0f;
      }

      if ((column == gameState->playerP.absTileX) &&
          (row == gameState->playerP.absTileY)) {
        gray = 0.0f;
      }

      real32 minX = centerX + ((real32)relColumn) * world.tileSideInPixels;
      real32 minY = centerY - ((real32)relRow) * world.tileSideInPixels;
      real32 maxX = minX + world.tileSideInPixels;
      real32 maxY = minY - world.tileSideInPixels;

      drawRectangle(buffer, minX, maxY, maxX, minY, gray, gray, gray);
    }
  }

  real32 playerR = 1.0f;
  real32 playerG = 1.0f;
  real32 playerB = 0.0f;

  real32 playerLeft =  centerX + world.metersToPixels * gameState->playerP.tileRelX -
                      (world.metersToPixels * playerWidth * 0.5f);
  real32 playerTop =  centerY - world.metersToPixels * gameState->playerP.tileRelY -
                     (world.metersToPixels * playerHeight);
  drawRectangle(buffer, playerLeft, playerTop,
                playerLeft + world.metersToPixels * playerWidth,
                playerTop + world.metersToPixels * playerHeight, playerR,
                playerG, playerB);
}

/*
internal void renderWeirdGradient(game_offscreen_buffer *buffer, int xOffset,
                                  int yOffset) {
  uint8 *row = (uint8 *)buffer->memory;
  for (int y = 0; y < buffer->height; ++y) {
    uint32 *pixel = (uint32 *)row;
    for (int x = 0; x < buffer->width; ++x) {
      uint8 blue = (uint8)(x + xOffset);
      uint8 green = (uint8)(y + yOffset);
      *pixel++ = (green << 8) | blue;
    }
    row += buffer->pitch;
  }
}
*/
