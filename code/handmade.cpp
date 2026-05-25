#include "handmade.h"
#include <cstdio>

internal void renderWeirdGradient(game_offscreen_buffer *buffer, int xOffset,
                                  int yOffset) {
  uint8 *row = (uint8 *)buffer->memory;
  for (int y = 0; y < buffer->height; ++y) {
    uint32 *pixel = (uint32 *)row;
    for (int x = 0; x < buffer->width; ++x) {
      uint8 blue = x + xOffset;
      uint8 green = y + yOffset;
      *pixel++ = (green << 8) | blue;
    }
    row += buffer->pitch;
  }
}

internal void gameOutputSound(game_sound_output_buffer *soundBuffer,
                              int toneHz) {

  local_persist real32 tSine = 0;
  int16 toneVolume = 3000;
  int wavePeriod = soundBuffer->samplesPerSecond / toneHz;
  int16 *sampleOut = soundBuffer->samples;

  for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount;
       ++sampleIndex) {

    real32 sineValue = sinf(tSine);

    int16 sampleValue = (int16)(sineValue * toneVolume);

    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;

    tSine += 2.0f * PI32 / (real32)wavePeriod;
    while (tSine > 2.0f * PI32) {
      tSine -= 2.0f * PI32;
    }
  }
}

internal void gameUpdateAndRender(game_memory *memory, game_input *input,
                                  game_offscreen_buffer *buffer,
                                  game_sound_output_buffer *soundBuffer) {

  assert(
      (int)(&input->controllers[0].terminator - &input->controllers[0].buttons[0]) ==
      (int)(arrayCount(input->controllers[0].buttons)));

  assert(sizeof(game_state) <= memory->permanentStorageSize);

  game_state *gameState = (game_state *)memory->permanentStorage;

  if (!memory->isInitialized) {
    char *filename = __FILE__;
    debug_read_file_result file = DEBUGPlatformReadEntireFile(filename);
    if (file.contents) {
      DEBUGPlatformWriteEntireFile("build/test.out", file.contentsSize,
                                   file.contents);
      DEBUGPlatformFreeFileMemory(&file);
    }
    gameState->blueOffset = 0;
    gameState->greenOffset = 0;
    gameState->toneHz = 256;
    memory->isInitialized = true;
  }
  for (int controllerIdx = 0;
       controllerIdx < (int)arrayCount(input->controllers); ++controllerIdx) {
    game_controller_input *controller = getController(input, controllerIdx);
    if (controller->isAnalog) {
    } else {
      if (controller->moveLeft.endedDown) {
        gameState->blueOffset -= 1;
      }

      if (controller->moveRight.endedDown) {
        gameState->blueOffset += 1;
      }
    }

    if (controller->actionDown.endedDown) {
      gameState->greenOffset++;
    }
  }

  gameOutputSound(soundBuffer, gameState->toneHz);
  renderWeirdGradient(buffer, gameState->blueOffset, gameState->greenOffset);
}
