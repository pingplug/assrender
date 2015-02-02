#ifndef _ASSRENDER_H_
#define _ASSRENDER_H_

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <ass/ass.h>
#include "avisynth_c.h"

#if defined(_MSC_VER)
#define __NO_ISOCEXT
#define __NO_INLINE__

#define strcasecmp _stricmp
#define atoll _atoi64
#endif

enum csp {
    BT601,
    BT709
};

enum plane { Y, U, V };

typedef struct {
    uint8_t* uv_tmp[2];
    struct lbounds {
        uint16_t start;
        uint16_t end;
    }*lbounds;
    uint32_t isvfr;
    ASS_Track* ass;
    ASS_Library* ass_library;
    ASS_Renderer* ass_renderer;
    int64_t* timestamp;
    enum csp colorspace;
} udata;

#endif
