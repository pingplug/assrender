#include "render.h"

void setbounds(udata* ud, int starty, int endy, int startx, int endx)
{
    int i;
    starty >>= 1;
    endy = (endy + 1) >> 1;
    startx >>= 1;
    endx = (endx + 1) >> 1;

    for (i = starty; i < endy; i++) {
        struct lbounds* ll = ud->lbounds + i;

        if (startx < ll->start)
            ll->start = startx;

        if (endx > ll->end)
            ll->end = endx;
    }
}

void blit_rgb(uint8_t* data, ASS_Image* img, uint32_t pitch, uint32_t height,
              uint32_t numc)
{
    while (img) {
        uint8_t* dst;
        uint32_t dst_delta;
        int x, y, k, c = 0;
        uint8_t a, r, g, b;
        uint8_t outa;
        uint8_t* sp = img->bitmap;

        if (img->w == 0 || img->h == 0) {
            img = img->next;
            continue;
        }

        // Move destination pointer to the bottom right corner of the
        // bounding box that contains the current overlay bitmap.
        // Remember that avisynth RGB bitmaps are upside down, hence we
        // need to render upside down.
        dst =
            data + (pitch * (height - img->dst_y - 1)) + img->dst_x * numc;
        dst_delta = pitch + img->w * numc;

        a = 255 - _a(img->color);
        r = _r(img->color);
        g = _g(img->color);
        b = _b(img->color);


        for (y = 0; y < img->h; y++) {
            for (x = 0; x < img->w; ++x) {
                k = div255(sp[x] * a);

                if (k && numc == 4) {
                    outa = scale(k, 255, dst[c + 3]);
                    dst[c    ] = dblend(k, b, dst[c + 3], dst[c    ], outa);
                    dst[c + 1] = dblend(k, g, dst[c + 3], dst[c + 1], outa);
                    dst[c + 2] = dblend(k, r, dst[c + 3], dst[c + 2], outa);
                    dst[c + 3] = div255(outa);
                } else {
                    dst[c    ] = blend(k, b, dst[c    ]);
                    dst[c + 1] = blend(k, g, dst[c + 1]);
                    dst[c + 2] = blend(k, r, dst[c + 2]);
                }

                c += numc;
            }

            dst -= dst_delta;
            sp += img->stride;
        }

        img = img->next;
    }
}

void blit444(ASS_Image* img, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
             uint32_t pitch, enum csp colorspace)
{
    uint8_t y, u, v, opacity, *src, *dsty, *dstu, *dstv;
    uint32_t k;

    int i, j;

    while (img) {
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

        opacity = 255 - _a(img->color);

        src = img->bitmap;
        dsty = dataY + img->dst_x + img->dst_y * pitch;
        dstu = dataU + img->dst_x + img->dst_y * pitch;
        dstv = dataV + img->dst_x + img->dst_y * pitch;

        if (img->w == 0 || img->h == 0) {
            img = img->next;
            continue;
        }

        for (i = 0; i < img->h; ++i) {
            for (j = 0; j < img->w; ++j) {
                k = div255(src[j] * opacity);
                dsty[j] = blend(k, y, dsty[j]);
                dstu[j] = blend(k, u, dstu[j]);
                dstv[j] = blend(k, v, dstv[j]);
            }

            src  += img->stride;
            dsty += pitch;

            dstu += pitch;
            dstv += pitch;
        }

        img = img->next;
    }
}

AVS_VideoFrame* AVSC_CC assrender_get_frame(AVS_FilterInfo* p, int n)
{
    udata* ud = (udata*)p->user_data;
    ASS_Track* ass = ud->ass;
    ASS_Renderer* ass_renderer = ud->ass_renderer;
    ASS_Image* img;
    AVS_VideoFrame* src;
    uint32_t height, pitch, pitchUV = 0;
    uint8_t *data, *dataY = 0, *dataU = 0, *dataV = 0;
    int64_t ts;

    src = avs_get_frame(p->child, n);

    avs_make_writable(p->env, &src);

    if (avs_is_planar(&p->vi)) {
        dataY = avs_get_write_ptr_p(src, AVS_PLANAR_Y);
        dataU = avs_get_write_ptr_p(src, AVS_PLANAR_U);
        dataV = avs_get_write_ptr_p(src, AVS_PLANAR_V);
        pitchUV = avs_get_pitch_p(src, AVS_PLANAR_U);
    }

    data = avs_get_write_ptr(src);

    height = avs_get_height(src);
    pitch = avs_get_pitch(src);

    if (!ud->isvfr) {
        // it’s a casting party!
        ts = (int64_t)n * (int64_t)1000 * (int64_t)p->vi.fps_denominator /
             (int64_t)p->vi.fps_numerator;
    } else {
        ts = ud->timestamp[n];
    }

    img = ass_render_frame(ass_renderer, ass, ts, NULL);

    if (img) {
        if (avs_is_rgb32(&p->vi)) { // RGBA
            blit_rgb(data, img, pitch, height, 4);
        } else if (avs_is_rgb24(&p->vi)) { // RGB
            blit_rgb(data, img, pitch, height, 3);
        } else if (avs_is_yuy2(&p->vi)) { // YUY2
            // TODO
        } else if (avs_is_planar(&p->vi)) {
            if (heightUV && heightUV < height) { // YV12
                int i, j;
                static ASS_Image* im;
                uint8_t *dstu, *dstv, *srcu, *srcv;
                uint8_t *dstu_next, *dstv_next, *srcu_next, *srcv_next;

                if (img) {
                    for (i = 0; i < (height + 1) >> 1; i++) {
                        ud->lbounds[i].start = 65535;
                        ud->lbounds[i].end = 0;
                    }

                    for (im = img; im; im = im->next)
                        setbounds(ud, im->dst_y, im->dst_y + im->h, im->dst_x, im->dst_x + im->w);

                    dstu = ud->uv_tmp[0];
                    dstv = ud->uv_tmp[1];
                    srcu = dataU;
                    srcv = dataV;

                    for (i = 0; i < (height + 1) >> 1; i++) {
                        struct lbounds* lb = ud->lbounds + i;
                        dstu_next = dstu + pitch;
                        dstv_next = dstv + pitch;

                        for (j = lb->start; j < lb->end; j++) {
                            dstu[j << 1]
                            = dstu[(j << 1) + 1]
                            = dstu_next[j << 1]
                            = dstu_next[(j << 1) + 1]
                            = srcu[j];

                            dstv[j << 1]
                            = dstv[(j << 1) + 1]
                            = dstv_next[j << 1]
                            = dstv_next[(j << 1) + 1]
                            = srcv[j];
                        }

                        srcu += pitchUV;
                        srcv += pitchUV;
                        dstu = dstu_next + pitch;
                        dstv = dstv_next + pitch;
                    }

                    blit444(img, dataY, ud->uv_tmp[0], ud->uv_tmp[1], pitch, ud->colorspace);

                    srcu = ud->uv_tmp[0];
                    srcv = ud->uv_tmp[1];
                    srcu_next = srcu + pitch;
                    srcv_next = srcv + pitch;
                    dstu = dataU;
                    dstv = dataV;

                    for (i = 0; i < (height + 1) >> 1; ++i) {
                        for (j = ud->lbounds[i].start; j < ud->lbounds[i].end; j++) {
                            dstu[j] = (
                                        srcu[j << 1]
                                        + srcu[(j << 1) + 1]
                                        + srcu_next[j << 1]
                                        + srcu_next[(j << 1) + 1]
                                      ) >> 2;

                            dstv[j] = (
                                        srcv[j << 1]
                                        + srcv[(j << 1) + 1]
                                        + srcv_next[j << 1]
                                        + srcv_next[(j << 1) + 1]
                                      ) >> 2;
                        }

                        dstu += pitchUV;
                        dstv += pitchUV;
                        srcu      = srcu_next + pitch;
                        srcu_next = srcu + pitch;
                        srcv      = srcv_next + pitch;
                        srcv_next = srcv + pitch;
                    }
                }
            } else if (pitchUV && pitchUV < pitch) { // YV16
                // TODO
            } else if (pitchUV == pitch) { // YV24
                blit444(img, dataY, dataU, dataV, pitch, ud->colorspace);
            } else { // Y8
                while (img) {
                    uint8_t y;

                    if (ud->colorspace == BT709) {
                        y = rgb2y709(img->color);
                    } else if (ud->colorspace == BT601) {
                        y = rgb2y601(img->color);
                    } else {
                        y = rgb2y2020(img->color);
                    }

                    uint8_t opacity = 255 - _a(img->color);

                    int i, j;

                    uint8_t* src = img->bitmap;
                    uint8_t* dsty = dataY + img->dst_x + img->dst_y * pitch;

                    if (img->w == 0 || img->h == 0) {
                        img = img->next;
                        continue;
                    }

                    for (i = 0; i < img->h; ++i) {
                        for (j = 0; j < img->w; ++j) {
                            uint32_t k = div255(src[j] * opacity);
                            dsty[j] = blend(k, y, dsty[j]);
                        }

                        src  += img->stride;
                        dsty += pitch;
                    }

                    img = img->next;
                }
            }
        }
    }

    return src;
}
