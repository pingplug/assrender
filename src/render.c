#include "render.h"

void make_sub_img_rgb(ASS_Image* img, uint8_t** sub_img, uint32_t width)
{
    uint8_t r, g, b, a, a1, *src, *dstR, *dstG, *dstB, *dstA;
    uint32_t i, j, dsta;

    while (img) {
        if (img->w == 0 || img->h == 0) {
            img = img->next;
            continue;
        }

        r = _r(img->color);
        g = _g(img->color);
        b = _b(img->color);
        a1 = 255 - _a(img->color);

        src = img->bitmap;
        dstR = sub_img[1] + img->dst_y * width + img->dst_x;
        dstG = sub_img[2] + img->dst_y * width + img->dst_x;
        dstB = sub_img[3] + img->dst_y * width + img->dst_x;
        dstA = sub_img[0] + img->dst_y * width + img->dst_x;

        for (i = 0; i < img->h; i++) {
            for (j = 0; j < img->w; j++) {
                a = div255(src[j] * a1);
                if (a) {
                    if (dstA[j]) {
                        dsta = scale(a, 255, dstA[j]);
                        dstR[j] = dblend(a, r, dstA[j], dstR[j], dsta);
                        dstG[j] = dblend(a, g, dstA[j], dstG[j], dsta);
                        dstB[j] = dblend(a, b, dstA[j], dstB[j], dsta);
                        dstA[j] = div255(dsta);
                    } else {
                        dstR[j] = r;
                        dstG[j] = g;
                        dstB[j] = b;
                        dstA[j] = a;
                    }
                    if (dstB[j] < 10 && dstR[j] < 10 && dstG[j] > 200)
                        fprintf(stderr, "%d %d  ", a, src[j]);
                }
            }

            src += img->stride;
            dstR += width;
            dstG += width;
            dstB += width;
            dstA += width;
        }

        img = img->next;
    }
}

void make_sub_img_yuv(ASS_Image* img, uint8_t** sub_img, uint32_t width, enum csp colorspace)
{
    uint8_t y, u, v, a, a1, *src, *dstY, *dstU, *dstV, *dstA;
    uint32_t i, j, dsta;

    while (img) {
        if (img->w == 0 || img->h == 0) {
            img = img->next;
            continue;
        }

        if (colorspace == BT709) {
            y = rgb2y709(img->color);
            u = rgb2u709(img->color);
            v = rgb2v709(img->color);
        } else if (colorspace == BT601) {
            y = rgb2y601(img->color);
            u = rgb2u601(img->color);
            v = rgb2v601(img->color);
        } else {
            y = rgb2y2020(img->color);
            u = rgb2u2020(img->color);
            v = rgb2v2020(img->color);
        }

        a1 = 255 - _a(img->color);

        src = img->bitmap;
        dstY = sub_img[1] + img->dst_y * width + img->dst_x;
        dstU = sub_img[2] + img->dst_y * width + img->dst_x;
        dstV = sub_img[3] + img->dst_y * width + img->dst_x;
        dstA = sub_img[0] + img->dst_y * width + img->dst_x;

        for (i = 0; i < img->h; i++) {
            for (j = 0; j < img->w; j++) {
                a = div255(src[j] * a1);
                if (a) {
                    if (dstA[j]) {
                        dsta = scale(a, 255, dstA[j]);
                        dstY[j] = dblend(a, y, dstA[j], dstY[j], dsta);
                        dstU[j] = dblend(a, u, dstA[j], dstU[j], dsta);
                        dstV[j] = dblend(a, v, dstA[j], dstV[j], dsta);
                        dstA[j] = div255(dsta);
                    } else {
                        dstY[j] = y;
                        dstU[j] = u;
                        dstV[j] = v;
                        dstA[j] = a;
                    }
                }
            }

            src += img->stride;
            dstY += width;
            dstU += width;
            dstV += width;
            dstA += width;
        }

        img = img->next;
    }
}

void make_sub_img_y(ASS_Image* img, uint8_t** sub_img, uint32_t width, enum csp colorspace)
{
    uint8_t y, a, a1, *src, *dstY, *dstA;
    uint32_t i, j, dsta;

    while (img) {
        if (img->w == 0 || img->h == 0) {
            img = img->next;
            continue;
        }

        if (colorspace == BT709) {
            y = rgb2y709(img->color);
        } else if (colorspace == BT601) {
            y = rgb2y601(img->color);
        } else {
            y = rgb2y2020(img->color);
        }

        a1 = 255 - _a(img->color);

        src = img->bitmap;
        dstY = sub_img[1] + img->dst_y * width + img->dst_x;
        dstA = sub_img[0] + img->dst_y * width + img->dst_x;

        for (i = 0; i < img->h; i++) {
            for (j = 0; j < img->w; j++) {
                a = div255(src[j] * a1);
                if (a) {
                    if (dstA[j]) {
                        dsta = scale(a, 255, dstA[j]);
                        dstY[j] = dblend(a, y, dstA[j], dstY[j], dsta);
                        dstA[j] = div255(dsta);
                    } else {
                        dstY[j] = y;
                        dstA[j] = a;
                    }
                }
            }

            src += img->stride;
            dstY += width;
            dstA += width;
        }

        img = img->next;
    }
}

void apply_rgba(uint8_t** sub_img, uint8_t* data, uint32_t pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcA, *srcR, *srcG, *srcB, *dstA, *dstR, *dstG, *dstB;
    uint32_t i, j, k, dsta;

    srcR = sub_img[1];
    srcG = sub_img[2];
    srcB = sub_img[3];
    srcA = sub_img[0];

    // Move destination pointer to the bottom right corner of the
    // bounding box that contains the current overlay bitmap.
    // Remember that avisynth RGB bitmaps are upside down, hence we
    // need to render upside down.
    dstB = data + pitch * (height - 1);
    dstG = dstB + 1;
    dstR = dstB + 2;
    dstA = dstB + 3;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (srcA[j]) {
                k = j * 4;
                dsta = scale(srcA[j], 255, dstA[k]);
                dstR[k] = dblend(srcA[j], srcR[j], dstA[k], dstR[k], dsta);
                dstG[k] = dblend(srcA[j], srcG[j], dstA[k], dstG[k], dsta);
                dstB[k] = dblend(srcA[j], srcB[j], dstA[k], dstB[k], dsta);
                dstA[k] = div255(dsta);
            }
        }

        srcR += width;
        srcG += width;
        srcB += width;
        srcA += width;
        dstR -= pitch;
        dstG -= pitch;
        dstB -= pitch;
        dstA -= pitch;
    }
}

void apply_rgb(uint8_t** sub_img, uint8_t* data, uint32_t pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcR, *srcG, *srcB, *srcA, *dstR, *dstG, *dstB;
    uint32_t i, j, k;

    srcR = sub_img[1];
    srcG = sub_img[2];
    srcB = sub_img[3];
    srcA = sub_img[0];

    // Move destination pointer to the bottom right corner of the
    // bounding box that contains the current overlay bitmap.
    // Remember that avisynth RGB bitmaps are upside down, hence we
    // need to render upside down.
    dstB = data + pitch * (height - 1);
    dstG = dstB + 1;
    dstR = dstB + 2;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (srcA[j]) {
                k = j * 3;
                dstR[k] = blend(srcA[j], srcR[j], dstR[k]);
                dstG[k] = blend(srcA[j], srcG[j], dstG[k]);
                dstB[k] = blend(srcA[j], srcB[j], dstB[k]);
            }
        }

        srcR += width;
        srcG += width;
        srcB += width;
        srcA += width;
        dstR -= pitch;
        dstG -= pitch;
        dstB -= pitch;
    }
}

void apply_yuy2(uint8_t** sub_img, uint8_t* data, uint32_t pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcY0, *srcY1, *srcU0, *srcU1, *srcV0, *srcV1, *srcA0, *srcA1;
    uint8_t *dstY0, *dstU, *dstY1, *dstV;
    uint32_t i, j, k;

    srcY0 = sub_img[1];
    srcY1 = sub_img[1] + 1;
    srcU0 = sub_img[2];
    srcU1 = sub_img[2] + 1;
    srcV0 = sub_img[3];
    srcV1 = sub_img[3] + 1;
    srcA0 = sub_img[0];
    srcA1 = sub_img[0] + 1;

    // YUYV
    dstY0 = data;
    dstU  = data + 1;
    dstY1 = data + 2;
    dstV  = data + 3;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j += 2) {
            if (srcA0[j] + srcA1[j]) {
                k = j * 2;
                dstY0[k] =  blend(srcA0[j], srcY0[j], dstY0[k]);
                dstY1[k] =  blend(srcA1[j], srcY1[j], dstY1[k]);
                dstU[k]  = blend2(srcA0[j], srcU0[j],
                                  srcA1[j], srcU1[j], dstU[k]);
                dstV[k]  = blend2(srcA0[j], srcV0[j],
                                  srcA1[j], srcV1[j], dstV[k]);
            }
        }

        srcY0 += width;
        srcY1 += width;
        srcU0 += width;
        srcU1 += width;
        srcV0 += width;
        srcV1 += width;
        srcA0 += width;
        srcA1 += width;
        dstY0 += pitch;
        dstU  += pitch;
        dstY1 += pitch;
        dstV  += pitch;
    }
}

void apply_yv12(uint8_t** sub_img, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
                uint32_t pitchY, uint32_t pitchUV, uint32_t width, uint32_t height)
{
    uint8_t *srcY00, *srcU00, *srcV00, *srcA00;
    uint8_t *srcY01, *srcU01, *srcV01, *srcA01;
    uint8_t *srcY10, *srcU10, *srcV10, *srcA10;
    uint8_t *srcY11, *srcU11, *srcV11, *srcA11;
    uint8_t *dstY00, *dstY01, *dstY10, *dstY11, *dstU, *dstV;
    uint32_t i, j, k;

    srcY00 = sub_img[1];
    srcY01 = sub_img[1] + 1;
    srcY10 = sub_img[1] + width;
    srcY11 = sub_img[1] + width + 1;
    srcU00 = sub_img[2];
    srcU01 = sub_img[2] + 1;
    srcU10 = sub_img[2] + width;
    srcU11 = sub_img[2] + width + 1;
    srcV00 = sub_img[3];
    srcV01 = sub_img[3] + 1;
    srcV10 = sub_img[3] + width;
    srcV11 = sub_img[3] + width + 1;
    srcA00 = sub_img[0];
    srcA01 = sub_img[0] + 1;
    srcA10 = sub_img[0] + width;
    srcA11 = sub_img[0] + width + 1;

    dstY00 = dataY;
    dstY01 = dataY + 1;
    dstY10 = dataY + pitchY;
    dstY11 = dataY + pitchY + 1;
    dstU   = dataU;
    dstV   = dataV;

    for (i = 0; i < height; i += 2) {
        for (j = 0; j < width; j += 2) {
            k = j >> 1;
            if (srcA00[j] + srcA01[j] + srcA10[j] + srcA11[j]) {
                dstY00[j] =  blend(srcA00[j], srcY00[j], dstY00[j]);
                dstY01[j] =  blend(srcA01[j], srcY01[j], dstY01[j]);
                dstY10[j] =  blend(srcA10[j], srcY10[j], dstY10[j]);
                dstY11[j] =  blend(srcA11[j], srcY11[j], dstY11[j]);
                dstU[k]   = blend4(srcA00[j], srcU00[j],
                                   srcA01[j], srcU01[j],
                                   srcA10[j], srcU10[j],
                                   srcA11[j], srcU11[j], dstU[k]);
                dstV[k]   = blend4(srcA00[j], srcV00[j],
                                   srcA01[j], srcV01[j],
                                   srcA10[j], srcV10[j],
                                   srcA11[j], srcV11[j], dstV[k]);
            }
        }

        srcY00 += width * 2;
        srcY01 += width * 2;
        srcY10 += width * 2;
        srcY11 += width * 2;
        srcU00 += width * 2;
        srcU01 += width * 2;
        srcU10 += width * 2;
        srcU11 += width * 2;
        srcV00 += width * 2;
        srcV01 += width * 2;
        srcV10 += width * 2;
        srcV11 += width * 2;
        srcA00 += width * 2;
        srcA01 += width * 2;
        srcA10 += width * 2;
        srcA11 += width * 2;
        dstY00 += pitchY * 2;
        dstY01 += pitchY * 2;
        dstY10 += pitchY * 2;
        dstY11 += pitchY * 2;
        dstU   += pitchUV;
        dstV   += pitchUV;
    }
}

void apply_yv16(uint8_t** sub_img, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
                uint32_t pitchY, uint32_t pitchUV, uint32_t width, uint32_t height)
{
    uint8_t *srcY0, *srcY1, *srcU0, *srcU1, *srcV0, *srcV1, *srcA0, *srcA1;
    uint8_t *dstY0, *dstU, *dstY1, *dstV;
    uint32_t i, j, k;

    srcY0 = sub_img[1];
    srcY1 = sub_img[1] + 1;
    srcU0 = sub_img[2];
    srcU1 = sub_img[2] + 1;
    srcV0 = sub_img[3];
    srcV1 = sub_img[3] + 1;
    srcA0 = sub_img[0];
    srcA1 = sub_img[0] + 1;

    // YUYV
    dstY0 = dataY;
    dstU  = dataU;
    dstY1 = dataY + 1;
    dstV  = dataV;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j += 2) {
            k = j >> 1;
            if (srcA0[j] + srcA1[j]) {
                dstY0[j] =  blend(srcA0[j], srcY0[j], dstY0[j]);
                dstY1[j] =  blend(srcA1[j], srcY1[j], dstY1[j]);
                dstU[k]  = blend2(srcA0[j], srcU0[j],
                                  srcA1[j], srcU1[j], dstU[k]);
                dstV[k]  = blend2(srcA0[j], srcV0[j],
                                  srcA1[j], srcV1[j], dstV[k]);
            }
        }

        srcY0 += width;
        srcY1 += width;
        srcU0 += width;
        srcU1 += width;
        srcV0 += width;
        srcV1 += width;
        srcA0 += width;
        srcA1 += width;
        dstY0 += pitchY;
        dstY1 += pitchY;
        dstU  += pitchUV;
        dstV  += pitchUV;
    }
}

void apply_yv24(uint8_t** sub_img, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
                uint32_t pitch, uint32_t width, uint32_t height)
{
    uint8_t *srcY, *srcU, *srcV, *srcA, *dstY, *dstU, *dstV;
    uint32_t i, j;

    srcY = sub_img[1];
    srcU = sub_img[2];
    srcV = sub_img[3];
    srcA = sub_img[0];

    dstY = dataY;
    dstU = dataU;
    dstV = dataV;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (srcA[j]) {
                dstY[j] = blend(srcA[j], srcY[j], dstY[j]);
                dstU[j] = blend(srcA[j], srcU[j], dstU[j]);
                dstV[j] = blend(srcA[j], srcV[j], dstV[j]);
            }
        }

        srcY += width;
        srcU += width;
        srcV += width;
        srcA += width;
        dstY += pitch;
        dstU += pitch;
        dstV += pitch;
    }
}

void apply_y8(uint8_t** sub_img, uint8_t* dataY, uint32_t pitch, uint32_t width,
              uint32_t height)
{
    uint8_t *srcY, *srcA, *dstY;
    uint32_t i, j;

    srcY = sub_img[1];
    srcA = sub_img[0];

    dstY = dataY;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (srcA[j]) {
                dstY[j] = blend(srcA[j], srcY[j], dstY[j]);
            }
        }

        srcY += width;
        srcA += width;
        dstY += pitch;
    }
}

AVS_VideoFrame* AVSC_CC assrender_get_frame(AVS_FilterInfo* p, int n)
{
    udata* ud = (udata*)p->user_data;
    ASS_Track* ass = ud->ass;
    ASS_Renderer* ass_renderer = ud->ass_renderer;
    ASS_Image* img;
    AVS_VideoFrame* src;
    uint32_t height, heightUV = 0, width, pitch, pitchUV = 0;
    uint8_t *data, *dataY = 0, *dataU = 0, *dataV = 0;
    int64_t ts;
    int changed;

    src = avs_get_frame(p->child, n);

    avs_make_writable(p->env, &src);

    if (avs_is_planar(&p->vi)) {
        dataY = avs_get_write_ptr_p(src, AVS_PLANAR_Y);
        dataU = avs_get_write_ptr_p(src, AVS_PLANAR_U);
        dataV = avs_get_write_ptr_p(src, AVS_PLANAR_V);
        pitchUV = avs_get_pitch_p(src, AVS_PLANAR_U);
        heightUV = avs_get_height_p(src, AVS_PLANAR_U);
    }

    data = avs_get_write_ptr(src);

    height = p->vi.height;
    width = p->vi.width;
    pitch = avs_get_pitch(src);

    if (!ud->isvfr) {
        // it’s a casting party!
        ts = (int64_t)n * (int64_t)1000 * (int64_t)p->vi.fps_denominator / (int64_t)p->vi.fps_numerator;
    } else {
        ts = ud->timestamp[n];
    }

    img = ass_render_frame(ass_renderer, ass, ts, &changed);

    if (img) {
        if (avs_is_rgb32(&p->vi)) { // RGBA
            if (changed) {
                memset(ud->sub_img[0], 0x00, height * width);
                make_sub_img_rgb(img, ud->sub_img, width);
            }

            apply_rgba(ud->sub_img, data, pitch, width, height);
        } else if (avs_is_rgb24(&p->vi)) { // RGB
            if (changed) {
                memset(ud->sub_img[0], 0x00, height * width);
                make_sub_img_rgb(img, ud->sub_img, width);
            }

            apply_rgb(ud->sub_img, data, pitch, width, height);
        } else if (avs_is_yuy2(&p->vi)) { // YUY2
            if (changed) {
                memset(ud->sub_img[0], 0x00, height * width);
                make_sub_img_yuv(img, ud->sub_img, width, ud->colorspace);
            }

            apply_yuy2(ud->sub_img, data, pitch, width, height);
        } else if (avs_is_planar(&p->vi)) {
            if (heightUV && heightUV < height) { // YV12
                if (changed) {
                    memset(ud->sub_img[0], 0x00, height * width);
                    make_sub_img_yuv(img, ud->sub_img, width, ud->colorspace);
                }

                apply_yv12(ud->sub_img, dataY, dataU, dataV, pitch, pitchUV, width, height);
            } else if (pitchUV && pitchUV < pitch) { // YV16
                if (changed) {
                    memset(ud->sub_img[0], 0x00, height * width);
                    make_sub_img_yuv(img, ud->sub_img, width, ud->colorspace);
                }

                apply_yv16(ud->sub_img, dataY, dataU, dataV, pitch, pitchUV, width, height);
            } else if (pitchUV == pitch) { // YV24
                if (changed) {
                    memset(ud->sub_img[0], 0x00, height * width);
                    make_sub_img_yuv(img, ud->sub_img, width, ud->colorspace);
                }

                apply_yv24(ud->sub_img, dataY, dataU, dataV, pitch, width, height);
            } else { // Y8
                if (changed) {
                    memset(ud->sub_img[0], 0x00, height * width);
                    make_sub_img_y(img, ud->sub_img, width, ud->colorspace);
                }

                apply_y8(ud->sub_img, dataY, pitch, width, height);
            }
        }
    }

    return src;
}
