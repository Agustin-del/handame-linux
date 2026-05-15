#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct linux32_offscreen_buffer {
  void *memory;
  uint16 width;
  uint16 height;
  int bytesPerPixel;
  int pitch;
};

struct linux32_audio_buffer {
  void *memory;
  uint32 size;
  uint16 bytesPerSample;
};

global_variable bool32 globalRunning;
global_variable linux32_offscreen_buffer globalBackbuffer;
global_variable linux32_audio_buffer globalAudioBuffer;

// Propias
global_variable int xOffset = 0;
global_variable int yOffset = 0;

#define ALSA_FUNCTION(name) global_variable typeof(name) *name##_

// INFO: CONTROL ALSA
ALSA_FUNCTION(snd_pcm_open);
#define snd_pcm_open snd_pcm_open_

ALSA_FUNCTION(snd_pcm_start);
#define snd_pcm_start snd_pcm_start_

ALSA_FUNCTION(snd_pcm_writei);
#define snd_pcm_writei snd_pcm_writei_

ALSA_FUNCTION(snd_pcm_avail);
#define snd_pcm_avail snd_pcm_avail_

ALSA_FUNCTION(snd_pcm_recover);
#define snd_pcm_recover snd_pcm_recover_

// INFO:CONFIG ALSA
ALSA_FUNCTION(snd_pcm_hw_params_malloc);
#define snd_pcm_hw_params_malloc snd_pcm_hw_params_malloc_
ALSA_FUNCTION(snd_pcm_hw_params_free);
#define snd_pcm_hw_params_free snd_pcm_hw_params_free_

ALSA_FUNCTION(snd_pcm_hw_params_any);
#define snd_pcm_hw_params_any snd_pcm_hw_params_any_

ALSA_FUNCTION(snd_pcm_hw_params);
#define snd_pcm_hw_params snd_pcm_hw_params_

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

internal snd_pcm_t *linux32InitSound(int32 samplesPerSecond,
                                     linux32_audio_buffer *audioBuffer) {
  void *alsaLib = dlopen("libasound.so.2", RTLD_NOW);
  if (alsaLib) {
    snd_pcm_open = (typeof(snd_pcm_open_))dlsym(alsaLib, "snd_pcm_open");
    snd_pcm_start = (typeof(snd_pcm_start_))dlsym(alsaLib, "snd_pcm_start");
    snd_pcm_writei = (typeof(snd_pcm_writei_))dlsym(alsaLib, "snd_pcm_writei");
    snd_pcm_recover =
        (typeof(snd_pcm_recover_))dlsym(alsaLib, "snd_pcm_recover");
    snd_pcm_avail = (typeof(snd_pcm_avail_))dlsym(alsaLib, "snd_pcm_avail");

    snd_pcm_hw_params_malloc = (typeof(snd_pcm_hw_params_malloc_))dlsym(
        alsaLib, "snd_pcm_hw_params_malloc");

    snd_pcm_hw_params_any =
        (typeof(snd_pcm_hw_params_any_))dlsym(alsaLib, "snd_pcm_hw_params_any");

    snd_pcm_hw_params =
        (typeof(snd_pcm_hw_params_))dlsym(alsaLib, "snd_pcm_hw_params");

    snd_pcm_hw_params_free = (typeof(snd_pcm_hw_params_free_))dlsym(
        alsaLib, "snd_pcm_hw_params_free");

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

    snd_pcm_hw_params_set_buffer_size =
        (typeof(snd_pcm_hw_params_set_buffer_size_))dlsym(
            alsaLib, "snd_pcm_hw_params_set_buffer_size");

    snd_pcm_t *pcm;
    if (!snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK,
                      SND_PCM_NONBLOCK)) {
      snd_pcm_hw_params_t *params;
      int error;
      error = snd_pcm_hw_params_malloc(&params);
      if (error) {
        return 0;
      }
      snd_pcm_hw_params_any(pcm, params);
      snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
      if (error) {
        return 0;
      }
      snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16_LE);
      if (error) {
        return 0;
      }
      snd_pcm_hw_params_set_channels(pcm, params, 2);
      if (error) {
        return 0;
      }
      snd_pcm_hw_params_set_rate(pcm, params, samplesPerSecond, 0);
      if (error) {
        return 0;
      }
      snd_pcm_hw_params_set_buffer_size(
          pcm, params, (audioBuffer->size / audioBuffer->bytesPerSample));
      if (error) {
        return 0;
      }
      if (!snd_pcm_hw_params(pcm, params)) {
        audioBuffer->memory = mmap(0, audioBuffer->size, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (audioBuffer->memory == MAP_FAILED) {
          return 0;
        }
        snd_pcm_hw_params_free(params);
        return pcm;
      } else {
      }
    } else {
    }
  } else {
  }
  return 0;
}

internal xcb_atom_t GetInternAtom(xcb_connection_t *conn, const char *name) {
  xcb_intern_atom_cookie_t cookie =
      xcb_intern_atom(conn, 0, strlen(name), name);
  xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, 0);
  xcb_atom_t atom = reply->atom;
  free(reply);
  return atom;
}

internal void renderWeirdGradient(linux32_offscreen_buffer *buffer, int xOffset,
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

internal void xResizeBackBuffer(linux32_offscreen_buffer *buffer, uint16 width,
                                uint16 height) {
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
  if (buffer->memory == MAP_FAILED) {
    printf("error alocando el backbuffer");
    return;
  }

  buffer->pitch = width * buffer->bytesPerPixel;
}

internal void xDisplayBufferInWindow(linux32_offscreen_buffer *buffer,
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

internal void xHandleEvents(xcb_connection_t *conn, uint8 depth,
                            xcb_generic_event_t *event,
                            xcb_gcontext_t gContext) {
  if (event) {
    switch (event->response_type & ~0x80) {
    case XCB_KEY_RELEASE:
    case XCB_KEY_PRESS: {

      xcb_key_press_event_t *ke = (xcb_key_press_event_t *)event;
      bool32 isPressed = (event->response_type & ~0x80) == XCB_KEY_PRESS;
      // TODO:manejar alt + f4
      switch (ke->detail) {
      // 24 == 'Q'
      case 24: {
      } break;
        // 25 == 'W'
      case 25: {

      } break;
        // 26 == 'E'
      case 26: {

      } break;
        // 38 == 'A'
      case 38: {

      } break;
        // 39 == 'S'
      case 39: {

      } break;
        // 40 == 'D'
      case 40: {

      } break;
        // 65 == SPACE
      case 65: {

      } break;
        // 111 == UP
      case 111: {
        if (isPressed) {
          yOffset -= 2;
        }

      } break;
        // 113 == LEFT
      case 113: {
        if (isPressed) {
          xOffset -= 2;
        }

      } break;
        // 114 == RIGHT
      case 114: {
        if (isPressed) {
          xOffset += 2;
        }

      } break;
        // 116 == DOWN
      case 116: {
        if (isPressed) {
          yOffset += 2;
        }
      } break;
      }
    } break;
    case XCB_FOCUS_IN: {
    } break;
    case XCB_FOCUS_OUT: {
    } break;
    case XCB_EXPOSE: {
      xcb_expose_event_t *ee = (xcb_expose_event_t *)event;
      if (ee->count == 0) {
        xDisplayBufferInWindow(&globalBackbuffer, conn, ee->window, depth,
                               gContext);
      }
    } break;
    case XCB_CONFIGURE_NOTIFY: {
    } break;
    default: {

    } break;
    }
  }
}

// fijarse si cambiando la fase(creo que se llama asi se empieza a reproducir al
// inicio)
int main() {
  xcb_connection_t *conn = xcb_connect(0, 0);
  if (xcb_connection_has_error(conn)) {
    return 1;
  }
  xResizeBackBuffer(&globalBackbuffer, 1280, 720);
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
   * entender un poco mas aunque ahora me lo pone fullscreen, a mi me suena mal
   * semanticamente hablando.
   * */

  xcb_atom_t stateAtom = GetInternAtom(conn, "_NET_WM_STATE");
  xcb_atom_t modalAtom = GetInternAtom(conn, "_NET_WM_STATE_MODAL");
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, stateAtom,
                      XCB_ATOM_ATOM, 32, 1, &modalAtom);

  xcb_atom_t typeAtom = GetInternAtom(conn, "_NET_WM_WINDOW_TYPE");
  xcb_atom_t utilityAtom = GetInternAtom(conn, "_NET_WM_WINDOW_TYPE_NORMAL");
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, typeAtom,
                      XCB_ATOM_ATOM, 32, 1, &utilityAtom);

  xcb_atom_t protocolAtom = GetInternAtom(conn, "WM_PROTOCOLS");
  xcb_atom_t deleteAtom = GetInternAtom(conn, "WM_DELETE_WINDOW");
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, protocolAtom,
                      XCB_ATOM_ATOM, 32, 1, &deleteAtom);
  const char *title = "handmade hero";

  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
                      XCB_ATOM_STRING, 8, strlen(title), title);

  xcb_gcontext_t gContext = xcb_generate_id(conn);
  xcb_create_gc(conn, gContext, window, XCB_GC_FOREGROUND,
                &screen->black_pixel);

  xcb_map_window(conn, window);

  int samplesPerSecond = 48000;
  int toneHz = 440;
  int16 toneVolume = 1000;
  int squareWavePeriod = (samplesPerSecond / toneHz);
  int halfSquareWavePeriod = squareWavePeriod / 2;
  int bytesPerSample = 2 * sizeof(int16);
  int audioBufferSize = samplesPerSecond * bytesPerSample * 2;
  uint32 runningSampleIndex = 0;

  globalAudioBuffer.bytesPerSample = bytesPerSample;
  globalAudioBuffer.size = audioBufferSize;
  snd_pcm_t *audioHandler =
      linux32InitSound(samplesPerSecond, &globalAudioBuffer);
  // TODO: si no se cargo el so?
  // loguear o fijarse de no usarlo, porque no cargo, es decir
  // no tengo las funciones disponibles globalmente

  globalRunning = true;
  while (globalRunning) {
    xcb_generic_event_t *event;
    while ((event = xcb_poll_for_event(conn))) {
      if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
        xcb_client_message_event_t *cm = (xcb_client_message_event_t *)event;
        if (cm->type == protocolAtom && cm->data.data32[0] == deleteAtom) {
          globalRunning = false;
        }
      } else {
        xHandleEvents(conn, screen->root_depth, event, gContext);
      }
      free(event);
    }

    renderWeirdGradient(&globalBackbuffer, xOffset, yOffset);

    int samplesToWrite;
    if ((samplesToWrite = snd_pcm_avail(audioHandler)) >= 0) {
      int16 *sampleOut = (int16 *)globalAudioBuffer.memory;
      for (int sampleIndex = 0; sampleIndex < samplesToWrite; ++sampleIndex) {
        int16 sampleValue =
            ((runningSampleIndex++) / halfSquareWavePeriod % 2 ? toneVolume
                                                               : -toneVolume);

        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
      }

      snd_pcm_writei(audioHandler, globalAudioBuffer.memory, samplesToWrite);
    }

    xDisplayBufferInWindow(&globalBackbuffer, conn, window, screen->root_depth,
                           gContext);
  }

  xcb_disconnect(conn);
  // munmap
  return (0);
}
