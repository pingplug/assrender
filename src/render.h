#ifndef _RENDER_H_
#define _RENDER_H_

#include "assrender.h"

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

#define rgba2y601(c)  ( (( 263*_r(c) + 516*_g(c) + 100*_b(c)) >> 10) + 16  )
#define rgba2u601(c)  ( ((-152*_r(c) - 298*_g(c) + 450*_b(c)) >> 10) + 128 )
#define rgba2v601(c)  ( (( 450*_r(c) - 376*_g(c) -  73*_b(c)) >> 10) + 128 )

#define rgba2y709(c)  ( (( 745  *_r(c) + 2506 *_g(c) + 253 *_b(c) ) >> 12) + 16 )
#define rgba2u709(c)  ( ((-41   *_r(c) - 407  *_g(c) + 448 *_b(c) ) >> 10) + 128 )
#define rgba2v709(c)  ( (( 14336*_r(c) - 11051*_g(c) - 3285*_b(c) ) >> 15) + 128 )

#define blend(srcA, srcRGB, dstA, dstRGB, outA)  \
    (((srcA * 255 * srcRGB + (dstRGB * dstA * (255 - srcA))) / outA + 255) >> 8)

AVS_VideoFrame* AVSC_CC assrender_get_frame(AVS_FilterInfo* p, int n);

#endif
