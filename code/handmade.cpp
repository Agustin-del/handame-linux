#include "handmade.h"

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

internal void renderPlayer(game_offscreen_buffer *buffer, int playerX,
                           int playerY) {

  uint8 *endOfBuffer =
      (uint8 *)buffer->memory + (buffer->pitch * buffer->height);

  uint32 color = 0x00000000;
  int top = playerY;
  int bottom = playerY + 50;
  int left = playerX;
  int right = playerX + 50;
  for (int y = top; y < bottom; ++y) {
    uint32 *pixel =
        (uint32 *)((uint8 *)buffer->memory + left * buffer->bytesPerPixel +
                   y * buffer->pitch);
    for (int x = left; x < right; ++x) {
      if ((uint8 *)pixel < endOfBuffer && (uint8 *)pixel >= buffer->memory) {
        *pixel++ = color;
      }
    }
  }
}

internal void gameOutputSound(game_state *gameState,
                              game_sound_output_buffer *soundBuffer) {

  int16 toneVolume = 4000;
  int wavePeriod = soundBuffer->samplesPerSecond / (gameState->toneHz);
  int16 *sampleOut = soundBuffer->samples;

  for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount;
       ++sampleIndex) {

#if 1
    real32 sineValue = sinf(gameState->tSine);
    int16 sampleValue = (int16)(sineValue * toneVolume);
#else
    int16 sampleValue = 0;
#endif

    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;

    gameState->tSine += 2.0f * PI32 / (real32)wavePeriod;
    if (gameState->tSine > 2.0f * PI32) {
      gameState->tSine -= 2.0f * PI32;
    }
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
#if HANDMADE_INTERNAL
    char *filename = __FILE__;
    debug_read_file_result file =
        memory->DEBUGPlatformReadEntireFile(thread, filename);
    if (file.contents) {
      memory->DEBUGPlatformWriteEntireFile(thread, "build/test.out",
                                           file.contentsSize, file.contents);
      memory->DEBUGPlatformFreeFileMemory(thread, &file);
    }
#endif
    gameState->blueOffset = 0;
    gameState->greenOffset = 0;
    gameState->toneHz = 256;
    gameState->tSine = 0.0f;
    gameState->playerX = 100;
    gameState->playerY = 100;
    memory->isInitialized = true;
  }
  for (int controllerIdx = 0;
       controllerIdx < (int)arrayCount(input->controllers); ++controllerIdx) {
    game_controller_input *controller = getController(input, controllerIdx);
    if (controller->isAnalog) {
      if (controller->moveRight.endedDown) {
        gameState->blueOffset += 4;
      }
    }

    if (controller->moveRight.endedDown) {
      gameState->playerX += 4;
      gameState->blueOffset += 1;
    }
    if (controller->moveLeft.endedDown) {
      gameState->playerX -= 4;
    }

    if (controller->moveDown.endedDown) {
      gameState->playerY += 4;
    }
    if (controller->moveUp.endedDown) {
      gameState->playerY -= 4;
    }

    if (gameState->tJump > 0) {
      gameState->playerY += (int)(5.0f * sinf(0.5f * PI32 * gameState->tJump));
    }

    if (controller->actionUp.endedDown) {
      gameState->tJump = 4.0f;
    }

    gameState->tJump -= 0.025f;
  }

  renderWeirdGradient(buffer, gameState->blueOffset, gameState->greenOffset);

  renderPlayer(buffer, gameState->playerX, gameState->playerY);

  renderPlayer(buffer, input->mouseX, input->mouseY);
  for (int buttonIndex = 0; buttonIndex < (int)arrayCount(input->mouseButtons);
       ++buttonIndex) {
    if (input->mouseButtons[buttonIndex].endedDown) {

      renderPlayer(buffer, (10 + 20 * buttonIndex), 10);
    }
  }
}
