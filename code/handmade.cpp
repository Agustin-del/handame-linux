#include "handmade.h"

internal void renderWeirdGradient(game_offscreen_buffer *buffer, int xOffset,
                                  int yOffset) {
  uint8 *row = (uint8 *)buffer->memory;
  for (int y = 0; y < buffer->height; ++y) {
    uint32 *pixel = (uint32 *)row;
    for (int x = 0; x < buffer->width; ++x) {
      uint8 blue = (uint8)(x + xOffset);
      uint8 green = (uint8)(y + yOffset);
      *pixel++ = (green << 16) | blue;
    }
    row += buffer->pitch;
  }
}

internal void gameOutputSound(game_state *gameState, game_sound_output_buffer *soundBuffer) {

  int16 toneVolume = 4000;
  int wavePeriod = soundBuffer->samplesPerSecond / (gameState->toneHz);
  int16 *sampleOut = soundBuffer->samples;

  for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount;
       ++sampleIndex) {

    real32 sineValue = sinf(gameState->tSine);

    int16 sampleValue = (int16)(sineValue * toneVolume);

    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;

    gameState->tSine += 2.0f * PI32 / (real32)wavePeriod;
    if (gameState->tSine > 2.0f * PI32) {
      gameState->tSine -= 2.0f * PI32;
    }
  }
}

extern "C" GAME_GET_SOUND_SAMPLES(gameGetSoundSamples){
  assert(sizeof(game_state) <= memory->permanentStorageSize);
  game_state *gameState = (game_state *)memory->permanentStorage;
  gameOutputSound(gameState, soundBuffer);
}

extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender){

  assert((int)(&input->controllers[0].terminator -
               &input->controllers[0].buttons[0]) ==
         (int)(arrayCount(input->controllers[0].buttons)));

  assert(sizeof(game_state) <= memory->permanentStorageSize);

  game_state *gameState = (game_state *)memory->permanentStorage;

  if (!memory->isInitialized) {
#if HANDMADE_INTERNAL 
    char *filename = __FILE__;
    debug_read_file_result file = memory->DEBUGPlatformReadEntireFile(filename);
    if (file.contents) {
      memory->DEBUGPlatformWriteEntireFile("build/test.out", file.contentsSize,
                                   file.contents);
      memory->DEBUGPlatformFreeFileMemory(&file);
    }
#endif
    gameState->blueOffset = 0;
    gameState->greenOffset = 0;
    gameState->toneHz = 256;
    gameState->tSine = 0.0f;
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

    if (controller->actionUp.endedDown) {
      gameState->toneHz+=10;
    }

    if (controller->actionDown.endedDown) {
      gameState->greenOffset++;
      gameState->toneHz-=10;
    }
  }

  renderWeirdGradient(buffer, gameState->blueOffset, gameState->greenOffset);
}
