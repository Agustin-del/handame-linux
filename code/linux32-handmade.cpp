#include "handmade.h"
#include "linux32-handmade.h"
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <xcb/present.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

global_variable bool32 globalRunning;
global_variable bool32 globalPause;
global_variable linux32_offscreen_buffer globalBackbuffer;
global_variable linux32_sound_output soundOutput;

#define ALSA_FUNCTION(name) global_variable typeof(name) *name##_

// INFO: CONTROL ALSA
ALSA_FUNCTION(snd_pcm_open);
#define snd_pcm_open snd_pcm_open_

ALSA_FUNCTION(snd_pcm_start);
#define snd_pcm_start snd_pcm_start_

ALSA_FUNCTION(snd_pcm_mmap_begin);
#define snd_pcm_mmap_begin snd_pcm_mmap_begin_

ALSA_FUNCTION(snd_pcm_avail_update);
#define snd_pcm_avail_update snd_pcm_avail_update_

ALSA_FUNCTION(snd_pcm_mmap_commit);
#define snd_pcm_mmap_commit snd_pcm_mmap_commit_

ALSA_FUNCTION(snd_pcm_delay);
#define snd_pcm_delay snd_pcm_delay_

ALSA_FUNCTION(snd_pcm_recover);
#define snd_pcm_recover snd_pcm_recover_

ALSA_FUNCTION(snd_pcm_drop);
#define snd_pcm_drop snd_pcm_drop_

ALSA_FUNCTION(snd_pcm_close);
#define snd_pcm_close snd_pcm_close_

// INFO:CONFIG ALSA
ALSA_FUNCTION(snd_pcm_hw_params_malloc);
#define snd_pcm_hw_params_malloc snd_pcm_hw_params_malloc_

ALSA_FUNCTION(snd_pcm_hw_params_free);
#define snd_pcm_hw_params_free snd_pcm_hw_params_free_

ALSA_FUNCTION(snd_pcm_hw_params_any);
#define snd_pcm_hw_params_any snd_pcm_hw_params_any_

ALSA_FUNCTION(snd_pcm_hw_params_set_access);
#define snd_pcm_hw_params_set_access snd_pcm_hw_params_set_access_

ALSA_FUNCTION(snd_pcm_hw_params_set_format);
#define snd_pcm_hw_params_set_format snd_pcm_hw_params_set_format_

ALSA_FUNCTION(snd_pcm_hw_params_set_channels);
#define snd_pcm_hw_params_set_channels snd_pcm_hw_params_set_channels_

ALSA_FUNCTION(snd_pcm_hw_params_set_rate);
#define snd_pcm_hw_params_set_rate snd_pcm_hw_params_set_rate_

ALSA_FUNCTION(snd_pcm_hw_params_set_buffer_size);
#define snd_pcm_hw_params_set_buffer_size snd_pcm_hw_params_set_buffer_size_

ALSA_FUNCTION(snd_pcm_hw_params);
#define snd_pcm_hw_params snd_pcm_hw_params_

// ERROR ALSA
ALSA_FUNCTION(snd_strerror);
#define snd_strerror snd_strerror_

inline timespec linux32GetLastWriteTime(char *sourceSOName) {

  struct stat result;
  stat(sourceSOName, &result);
  return result.st_mtim;
}

internal linux32_game_code linux32LoadGameCode(char *sourceSOName,
                                               char *tempSOName) {
  linux32_game_code result = {};
  result.SOLastWriteTime = linux32GetLastWriteTime(sourceSOName);
  /*INFO: no copio porque el so abre el so y lo cierra despues de mapearlo
  Ademas no tuve mejor resultado copiando. Sigo teniendo el glitch.*/

  int SOFD = open(sourceSOName, O_RDONLY);
  int tempSO = creat(tempSOName, 0755);
  int ret = ioctl(tempSO, FICLONE, SOFD);

  result.gameCodeSO = dlopen(tempSOName, RTLD_NOW);
  if (result.gameCodeSO) {
    result.updateAndRender = (game_update_and_render *)dlsym(
        result.gameCodeSO, "gameUpdateAndRender");
    result.getSoundSamples = (game_get_sound_samples *)dlsym(
        result.gameCodeSO, "gameGetSoundSamples");

    result.isValid = (result.getSoundSamples && result.updateAndRender);
  }

  if (!result.isValid) {

    result.updateAndRender = 0;
    result.getSoundSamples = 0;
  }

  return result;
}

internal void linux32UnloadGameCode(linux32_game_code *gameCode) {
  if (gameCode->gameCodeSO) {
    dlclose(gameCode->gameCodeSO);
  }

  gameCode->isValid = false;
  gameCode->getSoundSamples = 0;
  gameCode->updateAndRender = 0;
}

internal snd_pcm_t *linux32InitSound(int framesPerSecond) {
  void *alsaLib = dlopen("libasound.so.2", RTLD_NOW);
  if (alsaLib) {
    snd_pcm_open = (typeof(snd_pcm_open_))dlsym(alsaLib, "snd_pcm_open");
    snd_pcm_start = (typeof(snd_pcm_start_))dlsym(alsaLib, "snd_pcm_start");

    snd_pcm_mmap_begin =
        (typeof(snd_pcm_mmap_begin_))dlsym(alsaLib, "snd_pcm_mmap_begin");

    snd_pcm_avail_update =
        (typeof(snd_pcm_avail_update_))dlsym(alsaLib, "snd_pcm_avail_update");

    snd_pcm_mmap_commit =
        (typeof(snd_pcm_mmap_commit_))dlsym(alsaLib, "snd_pcm_mmap_commit");

    snd_pcm_drop = (typeof(snd_pcm_drop_))dlsym(alsaLib, "snd_pcm_drop");

    snd_pcm_delay = (typeof(snd_pcm_delay_))dlsym(alsaLib, "snd_pcm_delay");

    snd_pcm_recover =
        (typeof(snd_pcm_recover_))dlsym(alsaLib, "snd_pcm_recover");

    snd_pcm_close = (typeof(snd_pcm_close_))dlsym(alsaLib, "snd_pcm_close");

    snd_pcm_hw_params_malloc = (typeof(snd_pcm_hw_params_malloc_))dlsym(
        alsaLib, "snd_pcm_hw_params_malloc");

    snd_pcm_hw_params_free = (typeof(snd_pcm_hw_params_free_))dlsym(
        alsaLib, "snd_pcm_hw_params_free");

    snd_pcm_hw_params_any =
        (typeof(snd_pcm_hw_params_any_))dlsym(alsaLib, "snd_pcm_hw_params_any");

    snd_pcm_hw_params_set_access = (typeof(snd_pcm_hw_params_set_access_))dlsym(
        alsaLib, "snd_pcm_hw_params_set_access");

    snd_pcm_hw_params_set_format = (typeof(snd_pcm_hw_params_set_format_))dlsym(
        alsaLib, "snd_pcm_hw_params_set_format");

    snd_pcm_hw_params_set_channels =
        (typeof(snd_pcm_hw_params_set_channels_))dlsym(
            alsaLib, "snd_pcm_hw_params_set_channels");

    snd_pcm_hw_params_set_rate = (typeof(snd_pcm_hw_params_set_rate_))dlsym(
        alsaLib, "snd_pcm_hw_params_set_rate");

    snd_pcm_hw_params_set_buffer_size =
        (typeof(snd_pcm_hw_params_set_buffer_size_))dlsym(
            alsaLib, "snd_pcm_hw_params_set_buffer_size");

    snd_pcm_hw_params =
        (typeof(snd_pcm_hw_params_))dlsym(alsaLib, "snd_pcm_hw_params");

    snd_strerror = (typeof(snd_strerror_))dlsym(alsaLib, "snd_strerror");

    snd_pcm_t *pcm;
    if (!snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK,
                      SND_PCM_NONBLOCK)) {

      snd_pcm_hw_params_t *params;
      if (!snd_pcm_hw_params_malloc(&params)) {
        snd_pcm_hw_params_any(pcm, params);
        snd_pcm_hw_params_set_access(pcm, params,
                                     SND_PCM_ACCESS_MMAP_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(pcm, params, 2);
        snd_pcm_hw_params_set_rate(pcm, params, framesPerSecond, 0);

        snd_pcm_hw_params_set_buffer_size(pcm, params, 3200);
        if ((snd_pcm_hw_params(pcm, params) >= 0)) {
          snd_pcm_hw_params_free(params);
          return pcm;

        } else {
          snd_pcm_hw_params_free(params);
        }
      } else {
      }

    } else {
    }
  } else {
  }
  return 0;
}

internal xcb_atom_t linux32GetInternAtom(xcb_connection_t *conn,
                                         const char *name) {
  xcb_intern_atom_cookie_t cookie =
      xcb_intern_atom(conn, 0, strlen(name), name);
  xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, 0);
  xcb_atom_t atom = reply->atom;
  free(reply);
  return atom;
}

#if HANDMADE_INTERNAL
DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory) {
  munmap(file->contents, file->contentsSize);
  file->contents = 0;
  file->contentsSize = 0;
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile) {
  debug_read_file_result result = {};
  int fd = open(filename, O_RDONLY);
  if (fd > 0) {
    struct stat stats;
    if (fstat(fd, &stats) == 0) {
      uint32 fileSize32 = safeTruncateUint64(stats.st_size);
      result.contents = mmap(0, fileSize32, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (result.contents != MAP_FAILED) {
        int rCount = read(fd, result.contents, fileSize32);
        if ((uint32)rCount == fileSize32) {
          result.contentsSize = fileSize32;
        } else {
          DEBUGPlatformFreeFileMemory(&result);
          result.contents = 0;
        }
      } else {
      }
    } else {
    }
    close(fd);
  } else {
  }

  return result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile) {
  bool32 result = false;
  int fd = creat(filename, 0644);
  if (fd > 0) {
    int wCount = write(fd, memory, memorySize);
    result = ((uint32)wCount == memorySize);
    close(fd);
  } else {
  }

  return result;
}

#endif
internal void linux32FillSoundBuffer(snd_pcm_t *audioHandler,
                                     snd_pcm_sframes_t framesToWrite,
                                     game_sound_output_buffer *soundBuffer) {
  int16 *sourceFrame = soundBuffer->samples;
  while (framesToWrite > 0) {
    const snd_pcm_channel_area_t *areas;
    snd_pcm_uframes_t offset;
    snd_pcm_uframes_t frames = framesToWrite;
    if (!(snd_pcm_mmap_begin(audioHandler, &areas, &offset, &frames) < 0)) {
      int16 *destFrame = (int16 *)(((uint8 *)areas->addr + (areas->first / 8) +
                                    (offset * areas->step / 8)));

      for (snd_pcm_uframes_t sampleIndex = 0; sampleIndex < frames;
           ++sampleIndex) {
        *destFrame++ = *sourceFrame++;
        *destFrame++ = *sourceFrame++;
      }

      int commited = snd_pcm_mmap_commit(audioHandler, offset, frames);
      framesToWrite -= commited;
    }
  }
}

internal void linux32XResizeBackBuffer(linux32_offscreen_buffer *buffer,
                                       uint16 width, uint16 height) {
  if (buffer->memory) {
    munmap(buffer->memory,
           buffer->height * buffer->width * buffer->bytesPerPixel);
  }

  buffer->bytesPerPixel = 4;
  buffer->width = width;
  buffer->height = height;

  uint32 bitMapMemorySize = width * height * buffer->bytesPerPixel;
  buffer->memory = mmap(0, bitMapMemorySize, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(buffer->memory != MAP_FAILED);

  buffer->pitch = width * buffer->bytesPerPixel;
}

// TODO:Investigar shm
internal void linux32XDisplayBufferInWindow(linux32_offscreen_buffer *buffer,
                                            xcb_connection_t *conn,
                                            xcb_window_t window, uint8 depth,
                                            xcb_gcontext_t gContext) {
  uint32 bitMapMemorySize =
      buffer->width * buffer->height * buffer->bytesPerPixel;

  xcb_put_image(conn, XCB_IMAGE_FORMAT_Z_PIXMAP, window, gContext,
                buffer->width, buffer->height, 0, 0, 0, depth, bitMapMemorySize,
                (uint8 *)buffer->memory);
  xcb_flush(conn);
}

internal void linux32ProcessKey(game_button_state *newState, bool32 isDown) {
  if (newState->endedDown != isDown) {
    newState->endedDown = isDown;
    ++newState->halfTransitionCount;
  }
}

internal void linux32BeginRecordingInput(linux32_state *linux32State,
                                         int inputRecordingIndex) {
  linux32State->inputRecordingIndex = inputRecordingIndex;
  char *filename = "build/foo.hmi";
  linux32State->recordingFD = creat(filename, 0644);
  uint32 bytesToRead = linux32State->totalSize;
  assert(linux32State->totalSize == bytesToRead);
  write(linux32State->recordingFD, linux32State->gameMemoryBlock, bytesToRead);
}

internal void linux32EndRecordingInput(linux32_state *linux32State) {
  close(linux32State->recordingFD);
  linux32State->inputRecordingIndex = 0;
}

internal void linux32RecordInput(linux32_state *linux32State,
                                 game_input *newInput) {
  write(linux32State->recordingFD, newInput, sizeof(*newInput));
}

// Se podria hacer con un solo fd creo. Y abrirlo con permisos read/write.
internal void linux32BeginInputPlayback(linux32_state *linux32State,
                                        int inputPlayingIndex) {
  linux32State->inputPlayingIndex = inputPlayingIndex;
  char *filename = "build/foo.hmi";
  linux32State->playbackFD = open(filename, O_RDONLY);
  uint32 bytesToWrite = linux32State->totalSize;
  assert(linux32State->totalSize == bytesToWrite);
  read(linux32State->playbackFD, linux32State->gameMemoryBlock, bytesToWrite);
}

internal void linux32EndPlaybackInput(linux32_state *linux32State) {
  close(linux32State->playbackFD);
  linux32State->inputPlayingIndex = 0;
}

internal void linux32PlaybackInput(linux32_state *linux32State,
                                   game_input *newInput) {
  int rCount = read(linux32State->playbackFD, newInput, sizeof(*newInput));

  if (rCount <= 0) {
    int playingIndex = linux32State->inputPlayingIndex;
    linux32EndPlaybackInput(linux32State);
    linux32BeginInputPlayback(linux32State, playingIndex);
    read(linux32State->playbackFD, newInput, sizeof(*newInput));
  } else {
  }
}

internal void
linux32ProcessKeyboardMessage(linux32_state *linux32State,
                              xcb_key_press_event_t *event,
                              game_controller_input *keyboardController) {

  bool32 isDown = (event->response_type == XCB_KEY_PRESS);
  switch (event->detail) {
#if HANDMADE_INTERNAL
  // 33 == 'P'
  case 33: {
    if (isDown) {
      globalPause = !globalPause;
    }
  } break;
#endif
  // 24 == 'Q'
  case 24: {
  } break;
    // 25 == 'W'
  case 25: {
    linux32ProcessKey(&keyboardController->moveUp, isDown);
  } break;
    // 26 == 'E'
  case 26: {

  } break;
    // 38 == 'A'
  case 38: {
    linux32ProcessKey(&keyboardController->moveLeft, isDown);
  } break;
    // 39 == 'S'
  case 39: {
    linux32ProcessKey(&keyboardController->moveDown, isDown);
  } break;
    // 40 == 'D'
  case 40: {
    linux32ProcessKey(&keyboardController->moveRight, isDown);
  } break;
    // 46 == 'L'
  case 46: {
    if (isDown) {
      if (linux32State->inputRecordingIndex == 0) {
        linux32BeginRecordingInput(linux32State, 1);
      } else {
        linux32EndRecordingInput(linux32State);
        linux32BeginInputPlayback(linux32State, 1);
      }
    }
  } break;
    // 111 == UP
  case 111: {
    linux32ProcessKey(&keyboardController->actionUp, isDown);
  } break;
    // 113 == LEFT
  case 113: {
    linux32ProcessKey(&keyboardController->actionLeft, isDown);
  } break;
    // 114 == RIGHT
  case 114: {
    linux32ProcessKey(&keyboardController->actionRight, isDown);
  } break;
    // 116 == DOWN
  case 116: {
    linux32ProcessKey(&keyboardController->actionDown, isDown);
  } break;
    // 65 == SPACE
  case 65: {
    linux32ProcessKey(&keyboardController->back, isDown);
  } break;
    // 66 == ESC
  case 66: {
    linux32ProcessKey(&keyboardController->start, isDown);
  } break;
  }
}

inline timespec linux32GetTimeSpec() {
  struct timespec result;
  clock_gettime(CLOCK_MONOTONIC_RAW, &result);
  return result;
}

inline uint64 linux32GetNanoSecondsElapsed(timespec start, timespec end) {
  uint64 endNs = ((end.tv_sec * 1000000000LL) + end.tv_nsec);
  uint64 lastNs = ((start.tv_sec * 1000000000LL) + start.tv_nsec);
  uint64 result = endNs - lastNs;
  return result;
}

int main() {
  /* INFO: tambien como otro hack, en vez de cortar los picos o fijarse por
  overflow
   * puedo setear el hilo a que corra en un solo nucleo
  #include <sched.h>
  cpu_set_t set;

  CPU_ZERO(&set);
  CPU_SET(0, &set);

  sched_setaffinity(0, sizeof(set), &set);

  */

  xcb_connection_t *conn = xcb_connect(0, 0);
  if (xcb_connection_has_error(conn)) {
    return 1;
  }

  linux32XResizeBackBuffer(&globalBackbuffer, 1280, 720);
  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_window_t window = xcb_generate_id(conn);
  xcb_event_mask_t events =
      (xcb_event_mask_t)(XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_EXPOSURE |
                         XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                         XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE);

  xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0,
                    screen->width_in_pixels, screen->height_in_pixels, 10,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                    XCB_CW_EVENT_MASK, &events);

  /*
   * INFO: esto si realmente algun dia quisiese hacerlo en serio tendria que
   * entender un poco mas aunque ahora me lo pone fullscreen, a mi me suena
   * mal semanticamente hablando.
   * */

  xcb_atom_t stateAtom = linux32GetInternAtom(conn, "_NET_WM_STATE");
  xcb_atom_t modalAtom = linux32GetInternAtom(conn, "_NET_WM_STATE_MODAL");
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, stateAtom,
                      XCB_ATOM_ATOM, 32, 1, &modalAtom);

  xcb_atom_t typeAtom = linux32GetInternAtom(conn, "_NET_WM_WINDOW_TYPE");
  xcb_atom_t utilityAtom =
      linux32GetInternAtom(conn, "_NET_WM_WINDOW_TYPE_NORMAL");
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, typeAtom,
                      XCB_ATOM_ATOM, 32, 1, &utilityAtom);

  xcb_atom_t protocolAtom = linux32GetInternAtom(conn, "WM_PROTOCOLS");
  xcb_atom_t deleteAtom = linux32GetInternAtom(conn, "WM_DELETE_WINDOW");
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, protocolAtom,
                      XCB_ATOM_ATOM, 32, 1, &deleteAtom);
  const char *title = "handmade hero";

  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
                      XCB_ATOM_STRING, 8, strlen(title), title);

  xcb_gcontext_t gContext = xcb_generate_id(conn);
  xcb_create_gc(conn, gContext, window, XCB_GC_FOREGROUND,
                &screen->black_pixel);

  xcb_map_window(conn, window);

  int monitorRefreshHz = 60;
  int gameUpdateHz = monitorRefreshHz / 2;
  uint64 targetNanoSecondsPerFrame = NS / gameUpdateHz;

  real32 targetSecondsPerFrame = 1.0f / gameUpdateHz;

  soundOutput.framesPerSecond = 48000;
  soundOutput.bytesPerFrame = sizeof(int16) * 2;
  soundOutput.safetyFrames = ((soundOutput.framesPerSecond * 2) / gameUpdateHz);

  // soundOutput.latencyFramesCount = soundOutput.framesPerSecond /
  // gameUpdateHz;
  /*
soundOutput.latencyFramesCount = (int)((real32)soundOutput.framesPerSecond /
                                     ((real32)gameUpdateHz * 2 / 3));
                                     */

  snd_pcm_t *audioHandler = linux32InitSound(soundOutput.framesPerSecond);

  // TODO: si no se cargo el so?
  // loguear o fijarse de no usarlo, porque no cargo, es decir
  // no tengo las funciones disponibles globalmente

  linux32_state linux32State = {};
  globalRunning = true;
  int16 *samples =
      (int16 *)mmap(0, soundOutput.framesPerSecond * soundOutput.bytesPerFrame,
                    PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  assert(samples != MAP_FAILED);

#if HANDMADE_INTERNAL
  timespec lastCounter = linux32GetTimeSpec();

  uint64 lastCycleCount = __rdtsc();

  void *baseAddress = (void *)terabytes(2);
#else
  void *baseAddress = 0;
#endif

  game_memory gameMemory = {};

  gameMemory.permanentStorageSize = megabytes(64);
  gameMemory.transientStorageSize = gigabytes(1);

  gameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
  gameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
  gameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;

  linux32State.totalSize =
      gameMemory.permanentStorageSize + gameMemory.transientStorageSize;

  linux32State.gameMemoryBlock =
      mmap(baseAddress, linux32State.totalSize, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  gameMemory.permanentStorage = linux32State.gameMemoryBlock;
  assert(gameMemory.permanentStorage != MAP_FAILED);

  gameMemory.transientStorage =
      ((uint8 *)gameMemory.permanentStorage + gameMemory.permanentStorageSize);

  bool32 isSoundPlaying = false;

  if (samples && gameMemory.permanentStorage && gameMemory.transientStorage) {
    game_input input[2] = {};
    game_input *newInput = &input[0];
    game_input *oldInput = &input[1];

    char *sourceSOName = "build/handmade.so";
    char *tempSOName = "build/handmade-tmp.so";
    linux32_game_code game = linux32LoadGameCode(sourceSOName, tempSOName);
    while (globalRunning) {
      timespec newWriteSO = linux32GetLastWriteTime(sourceSOName);
      if (newWriteSO.tv_sec != game.SOLastWriteTime.tv_sec ||
          newWriteSO.tv_nsec != game.SOLastWriteTime.tv_nsec) {
        linux32UnloadGameCode(&game);
        game = linux32LoadGameCode(sourceSOName, tempSOName);
        game.SOLastWriteTime = newWriteSO;
      }

      game_controller_input *oldKeyboardController = getController(oldInput, 0);
      game_controller_input *newKeyboardController = getController(newInput, 0);
      game_controller_input zeroController = {};
      *newKeyboardController = zeroController;
      newKeyboardController->isConnected = true;

      for (int buttonIdx = 0;
           buttonIdx < (int)arrayCount(oldKeyboardController->buttons);
           ++buttonIdx) {
        newKeyboardController->buttons[buttonIdx].endedDown =
            oldKeyboardController->buttons[buttonIdx].endedDown;
      }

      xcb_generic_event_t *event;
      while ((event = xcb_poll_for_event(conn))) {
        switch (event->response_type & ~0x80) {
        case XCB_CLIENT_MESSAGE: {
          xcb_client_message_event_t *cm = (xcb_client_message_event_t *)event;
          if (cm->type == protocolAtom && cm->data.data32[0] == deleteAtom) {
            globalRunning = false;
          }
        } break;
        case XCB_DESTROY_NOTIFY: {
          globalRunning = false;
        } break;
        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE: {
          linux32ProcessKeyboardMessage(&linux32State,
                                        (xcb_key_press_event_t *)event,
                                        newKeyboardController);
        } break;
        case XCB_FOCUS_IN: {

        } break;
        case XCB_FOCUS_OUT: {
        } break;
        case XCB_EXPOSE: {
          xcb_expose_event_t *ee = (xcb_expose_event_t *)event;
          if (ee->count == 0) {
            linux32XDisplayBufferInWindow(&globalBackbuffer, conn, ee->window,
                                          screen->root_depth, gContext);
          }
        } break;
        case XCB_CONFIGURE_NOTIFY: {
        } break;

        default: {

        } break;
        }
        free(event);
      }

      if (!globalPause) {
        // Creo que podria usar el mismo buffer en vez de crear espacio
        // va no estoy alocando nada nuevo, pero tampoco tengo nada distinto
        // entre el buffer de plataforma y el del juego.
        /*
        game_offscreen_buffer buffer = {};
        buffer.memory = globalBackbuffer.memory;
        buffer.width = globalBackbuffer.width;
        buffer.height = globalBackbuffer.height;
        buffer.bytesPerPixel = globalBackbuffer.bytesPerPixel;
        buffer.pitch = globalBackbuffer.pitch;
        */

        if (linux32State.inputRecordingIndex) {
          linux32RecordInput(&linux32State, newInput);
        }

        if (linux32State.inputPlayingIndex) {
          linux32PlaybackInput(&linux32State, newInput);
        }

        if (game.updateAndRender) {
          game.updateAndRender(&gameMemory, newInput,
                               (game_offscreen_buffer *)&globalBackbuffer);
        }

        snd_pcm_sframes_t avail = snd_pcm_avail_update(audioHandler);
        snd_pcm_sframes_t framesWanted = 0;
        if (avail >= 0) {
          snd_pcm_sframes_t delay;
          snd_pcm_delay(audioHandler, &delay);
#if HANDMADE_INTERNAL
          printf("delay: %ld avail: %ld\n", delay, avail);
#endif
          framesWanted = soundOutput.safetyFrames - delay;
          if (framesWanted > avail) {
            framesWanted = avail;
          }

          game_sound_output_buffer soundBuffer = {};
          soundBuffer.samplesPerSecond = soundOutput.framesPerSecond;
          soundBuffer.sampleCount = framesWanted;
          soundBuffer.samples = samples;
          if(game.getSoundSamples) {
            game.getSoundSamples(&gameMemory, &soundBuffer);
          }

          linux32FillSoundBuffer(audioHandler, framesWanted, &soundBuffer);
          if (!isSoundPlaying) {
            int err = snd_pcm_start(audioHandler);
            if (!err) {
              isSoundPlaying = true;
            }
          }
        }
#if HANDMADE_INTERNAL
        else {
          int err = snd_pcm_recover(audioHandler, avail, 0);
          isSoundPlaying = false;
        }

        // INFO: hack porque cuando cambia de core el tsc no se mantiene,
        // entonces tengo picos y overflows. Quizas es malisimo lo que hice de
        // los ifs, no se, no la tengo tan clara.

        timespec endCounter = linux32GetTimeSpec();

        uint64 nanoSecondsElapsedForWork =
            linux32GetNanoSecondsElapsed(lastCounter, endCounter);

        uint64 nanoSecondsElapsedForFrame = nanoSecondsElapsedForWork;
        if (nanoSecondsElapsedForFrame < targetNanoSecondsPerFrame) {
          uint64 remainingNs =
              targetNanoSecondsPerFrame - nanoSecondsElapsedForFrame;
          timespec targetSleep = {
              .tv_sec = (time_t)(remainingNs / NS),
              .tv_nsec = (int64)(remainingNs % NS),
          };
          timespec rem;
          nanosleep(&targetSleep, &rem);

          timespec testCounter = linux32GetTimeSpec();
          uint64 testNanoSeconds =
              linux32GetNanoSecondsElapsed(lastCounter, testCounter);
          if (testNanoSeconds > targetNanoSecondsPerFrame) {
          }
          while (nanoSecondsElapsedForFrame < targetNanoSecondsPerFrame) {
            timespec checkCounter = linux32GetTimeSpec();
            nanoSecondsElapsedForFrame =
                linux32GetNanoSecondsElapsed(lastCounter, checkCounter);
          }

        } else {
        }

        endCounter = linux32GetTimeSpec();
        real32 msPerFrame =
            (real32)linux32GetNanoSecondsElapsed(lastCounter, endCounter) /
            1000000.0f;
        real32 FPS = (real32)(1.0f / (real32)(msPerFrame / 1000.0f));
#endif

        linux32XDisplayBufferInWindow(&globalBackbuffer, conn, window,
                                      screen->root_depth, gContext);

        game_input *temp = newInput;
        newInput = oldInput;
        oldInput = temp;
#if HANDMADE_INTERNAL
        char textBuffer[256];
        uint64 endCycleCount = __rdtsc();
        uint64 cyclesElapsed = endCycleCount - lastCycleCount;
        if (endCycleCount < lastCycleCount || cyclesElapsed > 100000000) {
          int len = sprintf(textBuffer, "%.2fms/f, %.2ff/s, skipped\n",
                            msPerFrame, FPS);
          write(2, textBuffer, len);
          lastCycleCount = endCycleCount;
          lastCounter = endCounter;
          continue;
        }

        real32 MCPF = ((real32)cyclesElapsed) / (1000.0f * 1000.0f);

        int len = sprintf(textBuffer, "%.2fms/f, %.2ff/s, %.2fmc/f\n",
                          msPerFrame, FPS, MCPF);
        write(2, textBuffer, len);

        lastCounter = endCounter;

        lastCycleCount = endCycleCount;
#endif
      }
    }
  } else {
  }

  snd_pcm_drop(audioHandler);
  snd_pcm_close(audioHandler);
  xcb_disconnect(conn);
  return (0);
}
