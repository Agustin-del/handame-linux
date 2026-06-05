#include "handmade.h"
#include "handmade-intrinsics.h"

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

inline void recanonicalizeCoord(world_map *world, int32 tileCount,
                                int32 *tilemap, int32 *tile, real32 *tileRel) {

  int32 offset = floorReal32ToInt32(*tileRel / world->tileSideInMeters);

  *tile += offset;
  *tileRel -= offset * world->tileSideInMeters;

  assert(*tileRel >= 0);
  assert(*tileRel < world->tileSideInMeters);

  if (*tile < 0) {
    *tile += tileCount;
    --(*tilemap);
  }

  if (*tile >= tileCount) {
    *tile -= tileCount;
    ++(*tilemap);
  }
}

inline canonical_position recanonicalizePosition(world_map *world,
                                                 canonical_position pos) {

  canonical_position result = pos;

  recanonicalizeCoord(world, world->countX, &result.tilemapX, &result.tileX,
                      &result.tileRelX);
  recanonicalizeCoord(world, world->countY, &result.tilemapY, &result.tileY,
                      &result.tileRelY);

  return result;
}

internal bool32 isWorldPointEmpty(world_map *world, canonical_position canPos) {
  bool32 empty = false;

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

  world.tileSideInMeters = 1.4f;
  world.tileSideInPixels = 60;
  world.metersToPixels = (real32)world.tileSideInPixels / world.tileSideInMeters;
  world.upperLeftX = -(real32)world.tileSideInPixels / 2;
  world.upperLeftY = 0.0f;

  real32 playerHeight = 1.4f; 
  real32 playerWidth = 0.75f * playerHeight;

  world.tilemaps = (tile_map *)tilemaps;

  game_state *gameState = (game_state *)memory->permanentStorage;

  if (!memory->isInitialized) {

    gameState->playerP.tilemapX = 0;
    gameState->playerP.tilemapY = 0;
    gameState->playerP.tileX = 3;
    gameState->playerP.tileY = 3;
    gameState->playerP.tileRelX = 0.4f;;
    gameState->playerP.tileRelY = 0.4f;
    memory->isInitialized = true;
  }

  tile_map *tilemap = getTileMap(&world, gameState->playerP.tilemapX,
                                 gameState->playerP.tilemapY);
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

      dPlayerX *= 2.0f;
      dPlayerY *= 2.0f;

      canonical_position newPlayerP = gameState->playerP;

      newPlayerP.tileRelX += input->dtForFrame * dPlayerX;
      newPlayerP.tileRelY += input->dtForFrame * dPlayerY;

      newPlayerP = recanonicalizePosition(&world, newPlayerP);

      canonical_position playerLeft = newPlayerP;
      playerLeft.tileRelX -= (playerWidth * 0.5f);
      playerLeft = recanonicalizePosition(&world, playerLeft);

      canonical_position playerRight = newPlayerP;
      playerRight.tileRelX += (playerWidth  * 0.5f);
      playerRight = recanonicalizePosition(&world, playerRight);

      if (isWorldPointEmpty(&world, newPlayerP) &&
          (isWorldPointEmpty(&world, playerLeft)) &&
          (isWorldPointEmpty(&world, playerRight))) {

        gameState->playerP = newPlayerP;
        /*
        gameState->playerP.tilemapX = canPos.tilemapX;
        gameState->playerP.tilemapY = canPos.tilemapY;

        gameState->playerP.tileRelX
        */
        /*
        gameState->playerP.tileRelX = world.upperLeftX +
                                      world.tileSideInPixels * canPos.tileX +
                                      canPos.tileRelX;
        gameState->playerP.tileRelY = world.upperLeftY +
                                      world.tileSideInPixels * canPos.tileY +
                                      canPos.tileRelY;
                                      */
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

      if((row == gameState->playerP.tileY) && (column == gameState->playerP.tileX)) {
        gray = 0.0f;
      }

      real32 minX =
          world.upperLeftX + ((real32)column)*world.tileSideInPixels;
      real32 minY = world.upperLeftY + ((real32)row)*world.tileSideInPixels;
      real32 maxX = minX + world.tileSideInPixels;
      real32 maxY = minY + world.tileSideInPixels;

      drawRectangle(buffer, minX, minY, maxX, maxY, gray, gray, gray);
    }
  }

  real32 playerR = 1.0f;
  real32 playerG = 1.0f;
  real32 playerB = 0.0f;

  real32 playerLeft = world.upperLeftX + gameState->playerP.tileX * world.tileSideInPixels +
                      world.metersToPixels * gameState->playerP.tileRelX - (world.metersToPixels * playerWidth * 0.5f);
  real32 playerTop = world.upperLeftY + gameState->playerP.tileY * world.tileSideInPixels +
                     world.metersToPixels * gameState->playerP.tileRelY - (world.metersToPixels * playerHeight);
  // real32 playerTop = (gameState->playerP.tileRelY - playerHeight);
  /*
  real32 playerLeft = (gameState->playerP.tileRelX - (playerWidth * 0.5f));
  real32 playerTop = (gameState->playerP.tileRelY - playerHeight);
  */
  drawRectangle(buffer, playerLeft, playerTop, playerLeft + world.metersToPixels * playerWidth,
                playerTop + world.metersToPixels * playerHeight, playerR, playerG, playerB);
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
