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

#define blend(srcA, srcC, dstC) \
    ((div255(srcA * srcC + (255 - srcA) * dstC)))
#define blend2(src1A, src1C, src2A, src2C, dstC) \
    ((div255(((src1A * src1C + src2A * src2C + (510 - src1A - src2A) * dstC + 1) >> 1))))
#define blend4(src1A, src1C, src2A, src2C, src3A, src3C, src4A, src4C, dstC) \
    ((div255(((src1A * src1C + src2A * src2C + src3A * src3C + src4A * src4C + (1020 - src1A - src2A - src3A - src4A) * dstC + 2) >> 2))))
#define scale(srcA, srcC, dstC) \
    ((srcA * srcC + (255 - srcA) * dstC))
#define dblend(srcA, srcC, dstA, dstC, outA) \
    (((srcA * srcC * 255 + dstA * dstC * (255 - srcA) + (outA >> 1)) / outA))

void col2rgb(uint32_t* c, uint8_t* r, uint8_t* g, uint8_t* b);
void col2yuv601(uint32_t* c, uint8_t* y, uint8_t* u, uint8_t* v);
void col2yuv709(uint32_t* c, uint8_t* y, uint8_t* u, uint8_t* v);
void col2yuv2020(uint32_t* c, uint8_t* y, uint8_t* u, uint8_t* v);

void apply_rgba(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_rgb(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yuy2(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yv12(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yv16(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_yv24(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);
void apply_y8(uint8_t** sub_img, uint8_t** data, uint32_t* pitch, uint32_t width, uint32_t height);

AVS_VideoFrame* AVSC_CC assrender_get_frame(AVS_FilterInfo* p, int n);

#endif
