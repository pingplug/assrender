#ifndef _RENDER_H_
#define _RENDER_H_

#include "assrender.h"

#define _r(c) (( (c) >> 24))
#define _g(c) ((((c) >> 16) & 0xFF))
#define _b(c) ((((c) >> 8)  & 0xFF))
#define _a(c) (( (c)        & 0xFF))

#define div256(x)   (((x + 128)   >> 8))
#define div65536(x) (((x + 32768) >> 16))
#define div255(x)   ((div256(x + div256(x))))
#define div65535(x) ((div65536(x + div65536(x))))

#define rgb2y601(c)  ((div65536(16829 * _r(c) + 33039 * _g(c) + 6416  * _b(c)) + 16))
#define rgb2u601(c)  ((div65536(-9714 * _r(c) - 19070 * _g(c) + 28784 * _b(c)) + 128))
#define rgb2v601(c)  ((div65536(28784 * _r(c) - 24103 * _g(c) - 4681  * _b(c)) + 128))

#define rgb2y709(c)  ((div65536(11966 * _r(c) + 40254 * _g(c) + 4064  * _b(c)) + 16))
#define rgb2u709(c)  ((div65536(-6596 * _r(c) - 22189 * _g(c) + 28784 * _b(c)) + 128))
#define rgb2v709(c)  ((div65536(28784 * _r(c) - 26145 * _g(c) - 2639  * _b(c)) + 128))

#define blend(srcA, srcC, dstC) \
    ((div255(srcA * srcC + (255 - srcA) * dstC)))
#define scale(srcA, srcC, dstC) \
    ((srcA * srcC + (255 - srcA) * dstC))
#define dblend(srcA, srcC, dstA, dstC, outA) \
    (((srcA * srcC * 255 + dstA * dstC * (255 - srcA) + (outA >> 1)) / outA))

AVS_VideoFrame* AVSC_CC assrender_get_frame(AVS_FilterInfo* p, int n);

#endif
