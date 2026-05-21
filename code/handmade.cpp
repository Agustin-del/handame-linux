#include "handmade.h"

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

internal void gameUpdateAndRender(game_input *input, game_offscreen_buffer *buffer,
                                  game_sound_output_buffer *soundBuffer) {
  local_persist int blueOffset = 0;
  local_persist int greenOffset = 0;
  local_persist int toneHz = 256;

  game_controller_input *input0 = &input->Controllers[0];
  if(input0->isAnalog) {

  } else {
    if (input0->left.endedDown) {
      blueOffset -= (int)(4.0f * 1.0f);
    } else if (input0->right.endedDown) {
      blueOffset += (int)(4.0f * 1.0f);
    } else if(input0->up.endedDown) {
      toneHz = 440 +(int(56.0f * 1.0f));
    } else if(input0->down.endedDown) {
      toneHz = 440 + (int(56.0f * -1.0f));
    }
  }

  if(input0->a.endedDown) {
    greenOffset += 1;
  }

  gameOutputSound(soundBuffer, toneHz);
  renderWeirdGradient(buffer, blueOffset, greenOffset);
}
