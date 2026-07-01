#include "handmade.h"
#include "handmade-intrinsics.h"
#include "handmade-platform.h"
#include "handmade-random.h"
#include "handmade-tile.cpp"

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

#pragma pack(push, 1)
struct bitmap_header {
  uint16 fileType;
  uint32 fileSize;
  uint16 reserved1;
  uint16 reserved2;
  uint32 bitmapOffset;
  uint32 size;
  int32 width;
  int32 height;
  uint16 planes;
  uint16 bitsPerPixel;
  uint32 compression;
  uint32 sizeOfBitmap;
  int32 horzResolution;
  int32 vertResolution;
  uint32 colorsUsed;
  uint32 colorsImportant;

  uint32 redMask;
  uint32 greenMask;
  uint32 blueMask;
};
#pragma pack(pop)

internal
    loaded_bitmap DEBUGloadBMP(thread_context *thread,
                               debug_platform_read_entire_file *readEntireFile,
                               char *filename) {
  loaded_bitmap result = {};
  debug_read_file_result readResult = readEntireFile(thread, filename);

  if (readResult.contentsSize != 0) {

    bitmap_header *header = (bitmap_header *)readResult.contents;
    uint32 *pixels =
        (uint32 *)((uint8 *)readResult.contents + header->bitmapOffset);

    result.pixels = pixels;
    result.width = header->width;
    result.height = header->height;

    assert(header->compression == 3);

    uint32 redMask = header->redMask;
    uint32 greenMask = header->greenMask;
    uint32 blueMask = header->blueMask;
    uint32 alphaMask =
        ~(header->redMask | header->greenMask | header->blueMask);

    bit_scan_result redShift = findLeastSignificantSetBit(redMask);
    bit_scan_result greenShift = findLeastSignificantSetBit(greenMask);
    bit_scan_result blueShift = findLeastSignificantSetBit(blueMask);
    bit_scan_result alphaShift = findLeastSignificantSetBit(alphaMask);

    assert(redShift.found);
    assert(greenShift.found);
    assert(blueShift.found);
    assert(alphaShift.found);

    uint32 *sourceDest = pixels;

    for (int32 y = 0; y < header->height; ++y) {
      for (int32 x = 0; x < header->width; ++x) {
        uint32 C = *sourceDest;
        *sourceDest++ = ((((C >> alphaShift.index) & 0xFF) << 24) |
                         (((C >> redShift.index) & 0xFF) << 16) |
                         (((C >> greenShift.index) & 0xFF) << 8) |
                         (((C >> blueShift.index) & 0xFF) << 0));
      }
    }
  }

  return result;
}

internal void drawBitmap(game_offscreen_buffer *buffer, loaded_bitmap *bitmap,
                         real32 realX, real32 realY) {

  int32 minX = roundReal32ToInt32(realX);
  int32 minY = roundReal32ToInt32(realY);
  int32 maxX = roundReal32ToInt32(realX + (real32)bitmap->width);
  int32 maxY = roundReal32ToInt32(realY + (real32)bitmap->height);

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

  uint32 *sourceRow = bitmap->pixels + bitmap->width * (bitmap->height - 1);
  uint8 *destRow = (uint8 *)buffer->memory + (buffer->pitch * minY) +
                   buffer->bytesPerPixel * minX;
  for (int32 y = minY; y < maxY; ++y) {
    uint32 *dest = (uint32 *)destRow;
    uint32 *source = sourceRow;
    for (int32 x = minX; x < maxX; ++x) {
      real32 a = (real32)((*source >> 24) & 0xFF) / 255.0f;
      real32 sR = (real32)((*source >> 16) & 0xFF);
      real32 sG = (real32)((*source >> 8) & 0xFF);
      real32 sB = (real32)((*source >> 0) & 0xFF);

      real32 dR = (real32)((*dest >> 16) & 0xFF);
      real32 dG = (real32)((*dest >> 8) & 0xFF);
      real32 dB = (real32)((*dest >> 0) & 0xFF);

      real32 r = (1.0f - a) * dR + a * sR;
      real32 g = (1.0f - a) * dG + a * sG;
      real32 b = (1.0f - a) * dB + a * sB;

      *dest = (((uint32)(r + 0.5f) << 16) |
            ((uint32)(g + 0.5f) << 8)|
            ((uint32)(b + 0.5f) << 0));
      ++dest;
      ++source;
    }

    destRow += buffer->pitch;
    sourceRow -= bitmap->width;
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

extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender) {
  assert((int)(&input->controllers[0].terminator -
               &input->controllers[0].buttons[0]) ==
         (int)(arrayCount(input->controllers[0].buttons)));

  assert(sizeof(game_state) <= memory->permanentStorageSize);

  game_state *gameState = (game_state *)memory->permanentStorage;

  if (!memory->isInitialized) {

    gameState->backdrop =
        DEBUGloadBMP(thread, memory->DEBUGPlatformReadEntireFile,
                     "data/test-background1.bmp");
    gameState->heroHead =
        DEBUGloadBMP(thread, memory->DEBUGPlatformReadEntireFile,
                     "data/test-hero-front-head.bmp");
    gameState->playerP.absTileX = 1;
    gameState->playerP.absTileY = 3;
    gameState->playerP.absTileZ = 0;

    gameState->playerP.offsetX = 5.0f;
    gameState->playerP.offsetY = 5.0f;
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
    tileMap->tileChunkCountZ = 2;

    tileMap->tileChunks =
        pushArray(&gameState->worldArena,
                  tileMap->tileChunkCountX * tileMap->tileChunkCountY *
                      tileMap->tileChunkCountZ,
                  tile_chunk);

    tileMap->tileSideInMeters = 1.4f;

    uint32 randomNumberIndex = 0;
    uint32 tilesPerWidth = 17;
    uint32 tilesPerHeight = 9;
    uint32 absTileZ = 0;
    uint32 screenX = 0;
    uint32 screenY = 0;

    bool32 doorLeft = false;
    bool32 doorRight = false;
    bool32 doorBottom = false;
    bool32 doorTop = false;
    bool32 doorUp = false;
    bool32 doorDown = false;

    for (uint32 screenIndex = 0; screenIndex < 100; ++screenIndex) {
      assert(randomNumberIndex < arrayCount(randomNumberTable));
      uint32 randomChoice;
      if (doorUp || doorDown) {
        randomChoice = randomNumberTable[randomNumberIndex++] % 2;
      } else {

        randomChoice = randomNumberTable[randomNumberIndex++] % 3;
      }

      bool32 createdZDoor = false;
      if (randomChoice == 2) {
        createdZDoor = true;
        if (absTileZ == 0) {
          doorUp = true;
        } else {
          doorDown = true;
        }
      } else if (randomChoice == 1) {
        doorRight = true;
      } else {
        doorTop = true;
      }

      for (uint32 tileY = 0; tileY < tilesPerHeight; ++tileY) {
        for (uint32 tileX = 0; tileX < tilesPerWidth; ++tileX) {
          uint32 absTileX = screenX * tilesPerWidth + tileX;
          uint32 absTileY = screenY * tilesPerHeight + tileY;

          uint32 tileValue = 1;
          if ((tileX == 0) && (!doorLeft || (tileY != tilesPerHeight / 2))) {

            tileValue = 2;
          }

          if ((tileX == tilesPerWidth - 1) &&
              (!doorRight || (tileY != tilesPerHeight / 2))) {
            tileValue = 2;
          }

          if ((tileY == 0) && (!doorBottom || (tileX != tilesPerWidth / 2))) {
            tileValue = 2;
          }

          if ((tileY == tilesPerHeight - 1) &&
              (!doorTop || (tileX != tilesPerWidth / 2))) {

            tileValue = 2;
          }

          if ((tileX == 10) && (tileY == 6)) {

            if (doorUp) {

              tileValue = 3;
            }

            if (doorDown) {

              tileValue = 4;
            }
          }
          setTileValue(&gameState->worldArena, world->tileMap, absTileX,
                       absTileY, absTileZ, tileValue);
        }
      }

      doorLeft = doorRight;
      doorBottom = doorTop;

      if (createdZDoor) {
        doorDown = !doorDown;
        doorUp = !doorUp;
      } else {
        doorUp = false;
        doorDown = false;
      }

      doorRight = false;
      doorTop = false;

      if (randomChoice == 2) {
        if (absTileZ == 0) {
          absTileZ = 1;
        } else {
          absTileZ = 0;
        }
      } else if (randomChoice == 1) {
        screenX += 1;
      } else {
        screenY += 1;
      }
    }
    memory->isInitialized = true;
  }

  world *world = gameState->world;
  tile_map *tileMap = world->tileMap;

  real32 playerHeight = 1.4f;
  real32 playerWidth = 0.75f * playerHeight;
  uint32 tileSideInPixels = 60;
  real32 metersToPixels = (real32)tileSideInPixels / tileMap->tileSideInMeters;

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

      newPlayerP.offsetX += input->dtForFrame * dPlayerX;
      newPlayerP.offsetY += input->dtForFrame * dPlayerY;

      newPlayerP = recanonicalizePosition(tileMap, newPlayerP);

      tile_map_position playerLeft = newPlayerP;
      playerLeft.offsetX -= (playerWidth * 0.5f);
      playerLeft = recanonicalizePosition(tileMap, playerLeft);

      tile_map_position playerRight = newPlayerP;
      playerRight.offsetX += (playerWidth * 0.5f);
      playerRight = recanonicalizePosition(tileMap, playerRight);

      if (isTileMapPointEmpty(tileMap, newPlayerP) &&
          (isTileMapPointEmpty(tileMap, playerLeft)) &&
          (isTileMapPointEmpty(tileMap, playerRight))) {

        if (!areOnSameTile(&gameState->playerP, &newPlayerP)) {
          uint32 newTileValue = getTileValue(tileMap, newPlayerP);
          if (newTileValue == 3) {
            ++newPlayerP.absTileZ;
          } else if (newTileValue == 4) {
            --newPlayerP.absTileZ;
          }
        }

        gameState->playerP = newPlayerP;
      }
    }
  }

  drawBitmap(buffer, &gameState->backdrop, 0, 0);

  real32 screenCenterX = (real32)buffer->width * 0.5f;
  real32 screenCenterY = (real32)buffer->height * 0.5f;

  for (int32 relRow = -10; relRow < 10; ++relRow) {
    for (int32 relColumn = -20; relColumn < 20; ++relColumn) {

      uint32 column = relColumn + gameState->playerP.absTileX;
      uint32 row = relRow + gameState->playerP.absTileY;
      uint32 tileID =
          getTileValue(tileMap, column, row, gameState->playerP.absTileZ);
      if (tileID > 1) {

        real32 gray = 0.5f;

        if (tileID == 2) {
          gray = 1.0f;
        }

        if (tileID > 2) {
          gray = 0.25f;
        }

        if ((column == gameState->playerP.absTileX) &&
            (row == gameState->playerP.absTileY)) {
          gray = 0.0f;
        }

        real32 centerX = screenCenterX +
                         ((real32)relColumn) * tileSideInPixels -
                         metersToPixels * gameState->playerP.offsetX;
        real32 centerY = screenCenterY - ((real32)relRow) * tileSideInPixels +
                         metersToPixels * gameState->playerP.offsetY;
        real32 minX = centerX - 0.5f * tileSideInPixels;
        real32 minY = centerY - 0.5f * tileSideInPixels;
        real32 maxX = centerX + 0.5f * tileSideInPixels;
        real32 maxY = centerY + 0.5f * tileSideInPixels;

        drawRectangle(buffer, minX, minY, maxX, maxY, gray, gray, gray);
      }
    }
  }

  real32 playerR = 1.0f;
  real32 playerG = 1.0f;
  real32 playerB = 0.0f;

  real32 playerLeft = screenCenterX - (metersToPixels * playerWidth * 0.5f);
  real32 playerTop = screenCenterY - (metersToPixels * playerHeight);
  drawRectangle(
      buffer, playerLeft, playerTop, playerLeft + metersToPixels * playerWidth,
      playerTop + metersToPixels * playerHeight, playerR, playerG, playerB);

  drawBitmap(buffer, &gameState->heroHead, playerLeft, playerTop);
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
