#ifndef HANDMADE_INTRINSICS

#include <math.h>

inline int32 roundReal32ToInt32(real32 real32) {
  int32 result = (int32)roundf(real32);
  return result;
}

inline uint32 roundReal32ToUint32(real32 real32) {
  uint32 result = (uint32)roundf(real32);
  return result;
}

inline int32 truncateReal32ToInt32(real32 real32) {
  int32 result = (int32)real32;
  return result;
}

inline int32 floorReal32ToInt32(real32 real32) {
  int32 result = (int32)floorf(real32);
  return result;
}

/*
inline real32 sin(real32 angle){
  real32 result = sinf(angle);
  return result;
}

inline real32 cos(real32 angle){
  real32 result = cosf(angle);
  return result;
}

inline real32 aTan2(real32 y, real32 x){
  real32 result = atan2(y, x);
  return result;
}
*/

struct bit_scan_result {
  bool32 found;
  uint32 index;
};

inline bit_scan_result findLeastSignificantSetBit(uint32 value) {

  bit_scan_result result = {};
#if defined(COMPILER_LLVM)
  if (value) {
    result.found = true;

    result.index = __builtin_ctz(value);
  }
#else
    for (uint32 test = 0; test < 32; ++test) {
      if (value & (1 << test)) {
        result.index = test;
        result.found = true;
        break;
      } else {
      }
    }
#endif
  return result;
}

#define HANDMADE_INTRINSICS
#endif
