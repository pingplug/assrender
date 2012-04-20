#include <ass/ass.h>
#include <stdint.h>
#include "avisynth_c.h"

#if defined(_MSC_VER)
#define __NO_ISOCEXT
#define __NO_INLINE__
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
// replacement of POSIX rint() for Windows
static int rint(double x)
{
    return floor(x + .5);
}

#define strcasecmp _stricmp
#define atoll _atoi64
#endif

enum csp {
    BT601,
    BT709
};

enum plane { Y, U, V };

typedef struct {
    uint8_t *uv_tmp[2];
    struct lbounds {
        uint16_t start;
        uint16_t end;
    } *lbounds;
    uint32_t isvfr;
    ASS_Track *ass;
    ASS_Library *ass_library;
    ASS_Renderer *ass_renderer;
    int64_t *timestamp;
    enum csp colorspace;
} udata;
