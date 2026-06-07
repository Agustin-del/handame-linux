#include "handmade.h"
#include "handmade-platform.h"
#include "handmade-tile.cpp"
#include "handmade-tile.h"

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

extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender) {
  assert((int)(&input->controllers[0].terminator -
               &input->controllers[0].buttons[0]) ==
         (int)(arrayCount(input->controllers[0].buttons)));

  assert(sizeof(game_state) <= memory->permanentStorageSize);
  real32 playerHeight = 1.4f;
  real32 playerWidth = 0.75f * playerHeight;

  game_state *gameState = (game_state *)memory->permanentStorage;

  if (!memory->isInitialized) {

    gameState->playerP.absTileX = 1;
    gameState->playerP.absTileY = 3;
    gameState->playerP.tileRelX = 5.0f;
    gameState->playerP.tileRelY = 5.0f;
    initializeArena(&gameState->worldArena,
                    memory->permanentStorageSize - sizeof(game_state),
                    (uint8 *)memory->permanentStorage + sizeof(game_state));

    gameState->world = pushStruct(&gameState->worldArena, world);

    world *world = gameState->world;

    world->tileMap = pushStruct(&gameState->worldArena, tile_map);

    tile_map *tileMap = world->tileMap;

    tileMap->chunkShift = 4;
    tileMap->chunkMask = (1 << tileMap->chunkShift) - 1;
    tileMap->chunkDim = (1 << tileMap->chunkShift);

    tileMap->tileChunkCountX = 128;
    tileMap->tileChunkCountY = 128;
    tileMap->tileChunks = pushArray(
        &gameState->worldArena,
        tileMap->tileChunkCountX * tileMap->tileChunkCountY, tile_chunk);

    for (uint32 x = 0; x < tileMap->tileChunkCountX; ++x) {
      for (uint32 y = 0; y < tileMap->tileChunkCountY; ++y) {
        tileMap->tileChunks[tileMap->tileChunkCountX * y + x].tiles =
            pushArray(&gameState->worldArena,
                      tileMap->chunkDim * tileMap->chunkDim, uint32);
      }
    }

    tileMap->tileSideInMeters = 1.4f;
    tileMap->tileSideInPixels = 60;
    tileMap->metersToPixels =
        (real32)tileMap->tileSideInPixels / tileMap->tileSideInMeters;

    uint32 tilesPerWidth = 17;
    uint32 tilesPerHeight = 9;
    for (uint32 screenY = 0; screenY < 32; ++screenY) {
      for (uint32 screenX = 0; screenX < 32; ++screenX) {
        for (uint32 tileY = 0; tileY < tilesPerHeight; ++tileY) {
          for (uint32 tileX = 0; tileX < tilesPerWidth; ++tileX) {
            uint32 absTileX = screenX * tilesPerWidth + tileX;
            uint32 absTileY = screenY * tilesPerHeight + tileY;
            setTileValue(&gameState->worldArena, world->tileMap, absTileX,
                         absTileY, (tileX == tileY) && (tileY % 2) ? 1 : 0);
          }
        }
      }
    }
    memory->isInitialized = true;
  }

  world *world = gameState->world;
  tile_map *tileMap = world->tileMap;
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
      real32 playerSpeed = 2.0f;

      if (controller->actionUp.endedDown) {
        playerSpeed = 10.0f;
      }
      dPlayerX *= playerSpeed;
      dPlayerY *= playerSpeed;

      tile_map_position newPlayerP = gameState->playerP;

      newPlayerP.tileRelX += input->dtForFrame * dPlayerX;
      newPlayerP.tileRelY += input->dtForFrame * dPlayerY;

      newPlayerP = recanonicalizePosition(tileMap, newPlayerP);

      tile_map_position playerLeft = newPlayerP;
      playerLeft.tileRelX -= (playerWidth * 0.5f);
      playerLeft = recanonicalizePosition(tileMap, playerLeft);

      tile_map_position playerRight = newPlayerP;
      playerRight.tileRelX += (playerWidth * 0.5f);
      playerRight = recanonicalizePosition(tileMap, playerRight);

      if (isTileMapPointEmpty(tileMap, newPlayerP) &&
          (isTileMapPointEmpty(tileMap, playerLeft)) &&
          (isTileMapPointEmpty(tileMap, playerRight))) {

        gameState->playerP = newPlayerP;
      }
    }
  }

  drawRectangle(buffer, 0.0f, 0.0f, buffer->width, buffer->height, 1.0f, 0.0f,
                1.0f);

  real32 screenCenterX = (real32)buffer->width * 0.5f;
  real32 screenCenterY = (real32)buffer->height * 0.5f;

  for (int32 relRow = -10; relRow < 10; ++relRow) {
    for (int32 relColumn = -20; relColumn < 20; ++relColumn) {

      uint32 column = relColumn + gameState->playerP.absTileX;
      uint32 row = relRow + gameState->playerP.absTileY;
      uint32 tileID = getTileValue(tileMap, column, row);
      real32 gray = 0.5f;

      if (tileID == 1) {
        gray = 1.0f;
      }

      if ((column == gameState->playerP.absTileX) &&
          (row == gameState->playerP.absTileY)) {
        gray = 0.0f;
      }

      real32 centerX = screenCenterX +
                       ((real32)relColumn) * tileMap->tileSideInPixels -
                       tileMap->metersToPixels * gameState->playerP.tileRelX;
      real32 centerY = screenCenterY -
                       ((real32)relRow) * tileMap->tileSideInPixels +
                       tileMap->metersToPixels * gameState->playerP.tileRelY;
      real32 minX = centerX - 0.5f * tileMap->tileSideInPixels;
      real32 minY = centerY - 0.5f * tileMap->tileSideInPixels;
      real32 maxX = centerX + 0.5f * tileMap->tileSideInPixels;
      real32 maxY = centerY + 0.5f * tileMap->tileSideInPixels;

      drawRectangle(buffer, minX, minY, maxX, maxY, gray, gray, gray);
    }
  }

  real32 playerR = 1.0f;
  real32 playerG = 1.0f;
  real32 playerB = 0.0f;

  real32 playerLeft =
      screenCenterX - (tileMap->metersToPixels * playerWidth * 0.5f);
  real32 playerTop = screenCenterY - (tileMap->metersToPixels * playerHeight);
  drawRectangle(buffer, playerLeft, playerTop,
                playerLeft + tileMap->metersToPixels * playerWidth,
                playerTop + tileMap->metersToPixels * playerHeight, playerR,
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
