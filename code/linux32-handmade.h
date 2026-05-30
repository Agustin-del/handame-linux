#ifndef LINUX32_HANDMADE_H

struct linux32_offscreen_buffer {
  void *memory;
  uint16 width;
  uint16 height;
  int bytesPerPixel;
  int pitch;
};

struct linux32_sound_output {
  int framesPerSecond;
  uint32 bytesPerFrame;
  int safetyFrames;
};

struct linux32_game_code {
  void *gameCodeSO;
  game_update_and_render *updateAndRender;
  game_get_sound_samples *getSoundSamples;
  timespec SOLastWriteTime;
  bool32 isValid;
};

struct linux32_state {
  uint64 totalSize;
  void *gameMemoryBlock;

  int recordingFD;
  int inputRecordingIndex;

  int playbackFD;
  int inputPlayingIndex;
};
#define LINUX32_HANDMADE_H
#endif
