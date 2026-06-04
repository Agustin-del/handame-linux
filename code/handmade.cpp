#include "handmade.h"

inline int32 roundReal32ToInt32(real32 real32) {
  int32 result = (int32)(real32 + 0.5f);
  return result;
}

inline uint32 roundReal32ToUint32(real32 real32) {
  uint32 result = (uint32)(real32 + 0.5f);
  return result;
}

inline int32 truncateReal32ToInt32(real32 real32) {
  int32 result = (int32)real32;
  return result;
}

#include <math.h>
inline int32 floorReal32ToInt32(real32 real32) {
  int32 result = (int32)floorf(real32);
  return result;
}

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

inline tile_map *getTileMap(world_map *world, int32 tilemapX, int32 tilemapY) {
  tile_map *tilemap = {};
  if ((tilemapX >= 0) && (tilemapX < world->tilemapCountX) && (tilemapY >= 0) &&
      (tilemapY < world->tilemapCountY)) {
    tilemap = &world->tilemaps[world->tilemapCountX * tilemapY + tilemapX];
  }
  return tilemap;
}

inline uint32 getTileValueUnchecked(world_map *world, tile_map *tilemap,
                                    int32 tileX, int32 tileY) {
  assert(tilemap);

  uint32 tilemapValue = tilemap->tiles[world->countX * tileY + tileX];
  return tilemapValue;
}

internal bool32 isTilemapPointEmpty(world_map *world, tile_map *tilemap,
                                    real32 testTileX, real32 testTileY) {

  bool32 empty = false;
  if (tilemap) {
    uint32 tilemapValue =
        getTileValueUnchecked(world, tilemap, testTileX, testTileY);
    if ((testTileX >= 0) && (testTileX < world->countX) && (testTileY >= 0) &&
        (testTileY < world->countY)) {

      empty = (tilemapValue == 0);
    }
  }

  return empty;
}

inline canonical_position getCanonicalPosition(world_map *world,
                                               raw_position pos) {

  canonical_position result;

  result.tilemapX = pos.tilemapX;
  result.tilemapY = pos.tilemapY;

  real32 x = pos.x - world->upperLeftX;
  real32 y = pos.y - world->upperLeftY;

  result.tileX = floorReal32ToInt32(x / world->tileWidth);
  result.tileY = floorReal32ToInt32(y / world->tileHeight);

  result.tileRelativeX = x - result.tileX * world->tileWidth;
  result.tileRelativeY = y - result.tileY * world->tileHeight;

  assert(result.tileRelativeX >= 0);
  assert(result.tileRelativeY >= 0);
  assert(result.tileRelativeX < world->tileWidth);
  assert(result.tileRelativeY < world->tileHeight);

  if (result.tileX < 0) {
    result.tileX += world->countX;
    --result.tilemapX;
  }

  if (result.tileY < 0) {
    result.tileY += world->countY;
    --result.tilemapY;
  }

  if (result.tileX >= world->countX) {
    result.tileX -= world->countX;
    ++result.tilemapX;
  }

  if (result.tileY >= world->countY) {
    result.tileY -= world->countY;
    ++result.tilemapY;
  }

  return result;
}

internal bool32 isWorldPointEmpty(world_map *world, raw_position testPos) {
  bool32 empty = false;

  canonical_position canPos = getCanonicalPosition(world, testPos);

  tile_map *tilemap = getTileMap(world, canPos.tilemapX, canPos.tilemapY);

  empty = isTilemapPointEmpty(world, tilemap, canPos.tileX, canPos.tileY);

  return empty;
}

extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender) {

  assert((int)(&input->controllers[0].terminator -
               &input->controllers[0].buttons[0]) ==
         (int)(arrayCount(input->controllers[0].buttons)));

  assert(sizeof(game_state) <= memory->permanentStorageSize);

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9

  // x = 0; y = 0;
  uint32 tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
      {1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
      {1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
      {1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1},
      {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
      {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
      {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
  };

  // x = 0; y = 1;
  uint32 tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
      {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  };

  uint32 tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
  };

  uint32 tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
      {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  };
  tile_map tilemaps[2][2];

  tilemaps[0][0].tiles = (uint32 *)tiles00;
  tilemaps[0][1].tiles = (uint32 *)tiles10;
  tilemaps[1][0].tiles = (uint32 *)tiles01;
  tilemaps[1][1].tiles = (uint32 *)tiles11;

  world_map world;
  world.tilemapCountX = 2;
  world.tilemapCountY = 2;

  world.countX = TILE_MAP_COUNT_X;
  world.countY = TILE_MAP_COUNT_Y;

  world.upperLeftX = -30.0f;
  world.upperLeftY = 0.0f;
  world.tileWidth = 60.0f;
  world.tileHeight = 60.0f;

  real32 playerWidth = 0.75f * world.tileWidth;
  real32 playerHeight = world.tileHeight;

  world.tilemaps = (tile_map *)tilemaps;

  game_state *gameState = (game_state *)memory->permanentStorage;

  if (!memory->isInitialized) {

    gameState->playerX = 150.0f;
    gameState->playerY = 150.0f;
    memory->isInitialized = true;
  }

  tile_map *tilemap =
      getTileMap(&world, gameState->playerTilemapX, gameState->playerTilemapY);
  assert(tilemap);

  for (int controllerIdx = 0;
       controllerIdx < (int)arrayCount(input->controllers); ++controllerIdx) {
    game_controller_input *controller = getController(input, controllerIdx);
    if (controller->isAnalog) {
    } else {
      real32 dPlayerX = 0.0f;
      real32 dPlayerY = 0.0f;

      if (controller->moveUp.endedDown) {
        dPlayerY = -1.0f;
      }
      if (controller->moveDown.endedDown) {
        dPlayerY = 1.0f;
      }

      if (controller->moveLeft.endedDown) {
        dPlayerX = -1.0f;
      }

      if (controller->moveRight.endedDown) {
        dPlayerX = 1.0f;
      }

      dPlayerX *= 64.0f;
      dPlayerY *= 64.0f;

      real32 newPlayerX = gameState->playerX + input->dtForFrame * dPlayerX;
      real32 newPlayerY = gameState->playerY + input->dtForFrame * dPlayerY;

      raw_position playerPos = {gameState->playerTilemapX,
                                gameState->playerTilemapY, newPlayerX,
                                newPlayerY};

      raw_position playerLeft = playerPos;
      playerLeft.x -= playerWidth * 0.5f;

      raw_position playerRight = playerPos;
      playerRight.x += playerWidth * 0.5f;

      if (isWorldPointEmpty(&world, playerPos) &&
          (isWorldPointEmpty(&world, playerLeft)) &&
          (isWorldPointEmpty(&world, playerRight))) {
        // tilemap = getTileMap(&world, gameState->playerTilemapX,
        // gameState->playerTilemapY);
        canonical_position canPos = getCanonicalPosition(&world, playerPos);
        gameState->playerTilemapX = canPos.tilemapX;
        gameState->playerTilemapY = canPos.tilemapY;
        gameState->playerX = world.upperLeftX + world.tileWidth * canPos.tileX +
                             canPos.tileRelativeX;
        gameState->playerY = world.upperLeftY +
                             world.tileHeight * canPos.tileY +
                             canPos.tileRelativeY;
      }
    }
  }

  drawRectangle(buffer, 0.0f, 0.0f, buffer->width, buffer->height, 1.0f, 0.0f,
                1.0f);

  for (int row = 0; row < world.countY; ++row) {
    for (int column = 0; column < world.countX; ++column) {

      uint32 tileID = getTileValueUnchecked(&world, tilemap, column, row);
      real32 gray = 0.5f;

      if (tileID == 1) {
        gray = 1.0f;
      }

      real32 minX = world.upperLeftX + ((real32)(column)*world.tileWidth);
      real32 minY = world.upperLeftY + ((real32)(row)*world.tileHeight);
      real32 maxX = minX + world.tileWidth;
      real32 maxY = minY + world.tileHeight;

      drawRectangle(buffer, minX, minY, maxX, maxY, gray, gray, gray);
    }
  }

  real32 playerR = 1.0f;
  real32 playerG = 1.0f;
  real32 playerB = 0.0f;
  real32 playerLeft = (gameState->playerX - (playerWidth * 0.5f));
  real32 playerTop = (gameState->playerY - playerHeight);
  drawRectangle(buffer, playerLeft, playerTop, playerLeft + playerWidth,
                playerTop + playerHeight, playerR, playerG, playerB);
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
