#include "avisynth_c.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <ass/ass.h>

#if defined(_MSC_VER)
#define __NO_ISOCEXT
#define __NO_INLINE__
#endif

#include <fontconfig/fontconfig.h>

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

#define rgba2y(c)  ( (( 263*_r(c) + 516*_g(c) + 100*_b(c)) >> 10) + 16  )
#define rgba2u(c)  ( ((-152*_r(c) - 298*_g(c) + 450*_b(c)) >> 10) + 128 )
#define rgba2v(c)  ( (( 450*_r(c) - 376*_g(c) -  73*_b(c)) >> 10) + 128 )

#define k121(a, b, c)  ((a + b + b + c + 3) >> 2)
#define blend(srcA, srcRGB, dstA, dstRGB, outA)  \
    (((srcA * 255 * srcRGB + (dstRGB * dstA * (255 - srcA))) / outA + 255) >> 8)

#if defined(_WIN32) && !defined(__MINGW32__)
// replacement of POSIX rint() for Windows
static int rint (double x) {
    return floor (x + .5);
}

#define strcasecmp _stricmp
#define atoll _atoi64
#endif

typedef struct {
    unsigned char *uv_tmp[2];
    struct lbounds {
        uint16_t start;
        uint16_t end;
    } *lbounds;
    unsigned int isvfr;
    ASS_Track *ass;
    ASS_Library *ass_library;
    ASS_Renderer *ass_renderer;
    int64_t *timestamp;
} udata;


int parse_timecodesv1 (FILE *f, int total, udata *ud) {
    int start, end, n = 0;
    double t = 0.0, basefps = 0.0, fps;
    char l[BUFSIZ];
    int64_t *ts = calloc (total, sizeof (int64_t));

    if (!ts)
        return 0;

    while ( (fgets (l, BUFSIZ - 1, f) != NULL) && n < total) {
        if (l[0] == 0 || l[0] == '\n' || l[0] == '\r' || l[0] == '#')
            continue;

        if (sscanf (l, "Assume %lf", &basefps) == 1)
            continue;

        if (!sscanf (l, "%d,%d,%lf", &start, &end, &fps) == 3)
            continue;

        if (basefps == 0.0)
            continue;

        while (n < start && n < total) {
            ts[n++] = (int64_t) rint (t);
            t += 1000.0 / basefps;
        }

        while (n <= end && n < total) {
            ts[n++] = (int64_t) rint (t);
            t += 1000.0 / fps;
        }
    }

    fclose (f);

    if (basefps == 0.0) {
        free (ts);
        return 0;
    }

    while (n < total) {
        ts[n++] = (int64_t) rint (t);
        t += 1000.0 / basefps;
    }

    ud->timestamp = ts;

    return 1;
}

int parse_timecodesv2 (FILE *f, int total, udata *ud) {
    int n = 0;
    int64_t *ts = calloc (total, sizeof (int64_t));
    char l[BUFSIZ];

    if (!ts) {
        return 0;
    }

    while ( (fgets (l, BUFSIZ - 1, f) != NULL) && n < total) {
        if (l[0] == 0 || l[0] == '\n' || l[0] == '\r' || l[0] == '#')
            continue;

        ts[n++] = atoll (l);
    }

    fclose (f);

    if (n < total) {
        free (ts);
        return 0;
    }

    ud->timestamp = ts;

    return 1;
}

ASS_Track *parse_srt (const char *f, udata *ud, const char *srt_font) {
    char l[BUFSIZ], buf[BUFSIZ];
    int start[4], end[4], isn;
    ASS_Track *ass = ass_new_track (ud->ass_library);
    FILE *fh = fopen (f, "r");

    if (!fh)
        return NULL;

    sprintf (buf, "[V4+ Styles]\nStyle: Default,%s,20,&H1EFFFFFF,&H00FFFFFF,"
             "&H29000000,&H3C000000,0,0,0,0,100,100,0,0,1,1,1.2,2,10,10,"
             "12,1\n\n[Events]\n",
             srt_font);

    ass_process_data (ass, buf, BUFSIZ - 1);

    while (fgets (l, BUFSIZ - 1, fh) != NULL) {
        if (l[0] == 0 || l[0] == '\n' || l[0] == '\r')
            continue;

        if (sscanf (l, "%d:%d:%d,%d --> %d:%d:%d,%d", &start[0], &start[1],
                    &start[2], &start[3], &end[0], &end[1], &end[2],
                    &end[3]) == 8) {
            sprintf (buf, "Dialogue: 0,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,"
                     "Default,,0,0,0,,{\\blur0.7}",
                     start[0], start[1], start[2],
                     (int) rint ( (double) start[3] / 10.0), end[0], end[1],
                     end[2], (int) rint ( (double) end[3] / 10.0));
            isn = 0;

            while (fgets (l, BUFSIZ - 1, fh) != NULL) {
                if (l[0] == 0 || l[0] == '\n' || l[0] == '\r')
                    break;

                if (l[strlen (l) - 1] == '\n' || l[strlen (l) - 1] == '\r')
                    l[strlen (l) - 1] = 0;

                if (isn) {
                    strcat (buf, "\\N");
                }

                strncat (buf, l, BUFSIZ - 1);
                isn = 1;
            }

            ass_process_data (ass, buf, BUFSIZ - 1);
        }
    }

    return ass;
}

void msg_callback (int level, const char *fmt, va_list va, void *data) {
    if (level > (int) data)
        return;

    fprintf (stderr, "libass: ");
    vfprintf (stderr, fmt, va);
    fprintf (stderr, "\n");
}

int init_ass (int w, int h, double scale, double line_spacing,
              ASS_Hinting hinting, double dar, double sar, int top,
              int bottom, int left, int right, int verbosity,
              const char *fontdir, udata *ud) {
    extern int FcDebugVal;
    ASS_Renderer *ass_renderer;
    ASS_Library *ass_library = ass_library_init();

    if (!ass_library)
        return 0;

    ass_set_message_cb (ass_library, msg_callback, (void *) verbosity);
    ass_set_extract_fonts (ass_library, 0);
    ass_set_style_overrides (ass_library, 0);

    ass_renderer = ass_renderer_init (ass_library);

    if (!ass_renderer)
        return 0;

    ass_set_font_scale (ass_renderer, scale);
    ass_set_hinting (ass_renderer, hinting);
    ass_set_frame_size (ass_renderer, w, h);
    ass_set_margins (ass_renderer, top, bottom, left, right);
    ass_set_use_margins (ass_renderer, 1);

    if (line_spacing)
        ass_set_line_spacing (ass_renderer, line_spacing);

    if (dar && sar)
        ass_set_aspect_ratio (ass_renderer, dar, sar);

    if (verbosity > 0) {
        // update Fontconfig’s cache verbosely
        FcDebugVal = 128;
    }

    // don’t scan for home directory as it’s not needed on win32
    FcConfigEnableHome (FcFalse);

    if (strcmp (fontdir, ""))
        ass_set_fonts_dir (ass_library, fontdir);

    ass_set_fonts (ass_renderer, NULL, NULL, 1, NULL, 1);
    FcDebugVal = 0;
    ud->ass_library = ass_library;
    ud->ass_renderer = ass_renderer;

    return 1;
}

void blit_rgb (unsigned char *data, ASS_Image *img, unsigned int pitch,
               unsigned int height, unsigned int numc) {
    while (img) {
        unsigned char *dst;
        unsigned int dst_delta;
        int x, y, k, c = 0;
        unsigned char a, r, g, b;
        unsigned char outa;
        unsigned char *sp = img->bitmap;

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

        a = 255 - _a (img->color);
        r = _r (img->color);
        g = _g (img->color);
        b = _b (img->color);


        for (y = 0; y < img->h; y++) {
            for (x = 0; x < img->w; ++x) {
                k = (sp[x] * a + 255) >> 8;

                if (k && numc == 4) {
                    outa = (k * 255 + (dst[c + 3] * (255 - k)) + 255) >> 8;
                    dst[c    ] = blend (k, b, dst[c + 3], dst[c    ], outa);
                    dst[c + 1] = blend (k, g, dst[c + 3], dst[c + 1], outa);
                    dst[c + 2] = blend (k, r, dst[c + 3], dst[c + 2], outa);
                    dst[c + 3] = outa;
                }
                else {
                    k = (sp[x] * a + 255) >> 8;
                    dst[c    ] = (k * b + (255 - k) * dst[c    ] + 255) >> 8;
                    dst[c + 1] = (k * g + (255 - k) * dst[c + 1] + 255) >> 8;
                    dst[c + 2] = (k * r + (255 - k) * dst[c + 2] + 255) >> 8;
                }

                c += numc;
            }

            dst -= dst_delta;
            sp += img->stride;
        }

        img = img->next;
    }
}

void blit444 (ASS_Image *img, unsigned char *dataY, unsigned char *dataU,
              unsigned char *dataV, unsigned int pitch) {
    while (img) {
        unsigned char y = rgba2y (img->color);
        unsigned char u = rgba2u (img->color);
        unsigned char v = rgba2v (img->color);
        unsigned char opacity = 255 - _a (img->color);

        int i, j;

        unsigned char *src = img->bitmap;
        unsigned char *dsty = dataY + img->dst_x + img->dst_y * pitch;
        unsigned char *dstu = dataU + img->dst_x + img->dst_y * pitch;
        unsigned char *dstv = dataV + img->dst_x + img->dst_y * pitch;

        if (img->w == 0 || img->h == 0) {
            img = img->next;
            continue;
        }

        for (i = 0; i < img->h; ++i) {
            for (j = 0; j < img->w; ++j) {
                unsigned int k = (src[j] * opacity + 255) >> 8;
                dsty[j] = (k * y + (255 - k) * dsty[j] + 255) >> 8;
                dstu[j] = (k * u + (255 - k) * dstu[j] + 255) >> 8;
                dstv[j] = (k * v + (255 - k) * dstv[j] + 255) >> 8;
            }

            src  += img->stride;
            dsty += pitch;

            dstu += pitch;
            dstv += pitch;
        }

        img = img->next;
    }
}

void setbounds (udata *ud, int starty, int endy,
                int startx, int endx) {
    int i;
    starty >>= 1;
    endy = (endy + 1) >> 1;
    startx >>= 1;
    endx = (endx + 1) >> 1;

    for (i = starty; i < endy; i++) {
        struct lbounds *ll = ud->lbounds + i;

        if (startx < ll->start)
            ll->start = startx;

        if (endx > ll->end)
            ll->end = endx;
    }
}

AVS_VideoFrame *AVSC_CC assrender_get_frame (AVS_FilterInfo *p, int n) {
    udata *ud = (udata *) p->user_data;
    ASS_Track *ass = ud->ass;
    ASS_Renderer *ass_renderer = ud->ass_renderer;
    ASS_Image *img;
    AVS_VideoFrame *src;
    unsigned int height, pitch, pitchUV = 0;
    unsigned char *data, *dataY = 0, *dataU = 0, *dataV = 0;
    int64_t ts;

    src = avs_get_frame (p->child, n);

    avs_make_writable (p->env, &src);

    if (avs_is_planar (&p->vi)) {
        dataY = avs_get_write_ptr_p (src, AVS_PLANAR_Y);
        dataU = avs_get_write_ptr_p (src, AVS_PLANAR_U);
        dataV = avs_get_write_ptr_p (src, AVS_PLANAR_V);
        pitchUV = avs_get_pitch_p (src, AVS_PLANAR_U);
    }

    data = avs_get_write_ptr (src);

    height = avs_get_height (src);
    pitch = avs_get_pitch (src);

    if (!ud->isvfr) {
        // it’s a casting party!
        ts = (int64_t) n * (int64_t) 1000 * (int64_t) p->vi.fps_denominator /
             (int64_t) p->vi.fps_numerator;
    }
    else {
        ts = ud->timestamp[n];
    }

    img = ass_render_frame (ass_renderer, ass, ts, NULL);

    if (avs_is_rgb32 (&p->vi) || avs_is_rgb24 (&p->vi)) {
        blit_rgb (data, img, pitch, height, avs_is_rgb32 (&p->vi) ? 4 : 3);
    }
    else if (avs_is_yuy2 (&p->vi)) {
        // TODO
    }
    else if (avs_is_yuv (&p->vi)) {

        if (avs_is_yv12 (&p->vi)) {
            int i, j;
            static ASS_Image *im;
            unsigned char *dstu, *dstv, *srcu, *srcv;
            unsigned char *dstu_next, *dstv_next, *srcu_next, *srcv_next;

            if (img) {
                for (i = 0; i < (height + 1) >> 1; i++) {
                    ud->lbounds[i].start = 65535;
                    ud->lbounds[i].end = 0;
                }

                for (im = img; im; im = im->next)
                    setbounds (ud, im->dst_y, im->dst_y + im->h,
                               im->dst_x, im->dst_x + im->w);

                dstu = ud->uv_tmp[0];
                dstv = ud->uv_tmp[1];
                srcu = dataU;
                srcv = dataV;

                for (i = 0; i < (height + 1) >> 1; i++) {
                    struct lbounds *lb = ud->lbounds + i;
                    dstu_next = dstu + pitch;
                    dstv_next = dstv + pitch;

                    for (j = lb->start; j < lb->end; j++) {
                        dstu[j << 1]
                        = dstu[ (j << 1) + 1]
                        = dstu_next[j << 1]
                        = dstu_next[ (j << 1) + 1]
                        = srcu[j];

                        dstv[j << 1]
                        = dstv[ (j << 1) + 1]
                        = dstv_next[j << 1]
                        = dstv_next[ (j << 1) + 1]
                        = srcv[j];
                    }

                    srcu += pitchUV;
                    srcv += pitchUV;
                    dstu = dstu_next + pitch;
                    dstv = dstv_next + pitch;
                }

                blit444 (img, dataY, ud->uv_tmp[0], ud->uv_tmp[1], pitch);

                srcu = ud->uv_tmp[0];
                srcv = ud->uv_tmp[1];
                srcu_next = srcu + pitch;
                srcv_next = srcv + pitch;
                dstu = dataU;
                dstv = dataV;

                for (i = 0; i < (height + 1) >> 1; ++i) {
                    for (j = ud->lbounds[i].start;
                            j < ud->lbounds[i].end; j++) {
                        dstu[j] = (
                                      srcu[j << 1]
                                      + srcu[ (j << 1) + 1]
                                      + srcu_next[j << 1]
                                      + srcu_next[ (j << 1) + 1]
                                  ) >> 2;

                        dstv[j] = (
                                      srcv[j << 1]
                                      + srcv[ (j << 1) + 1]
                                      + srcv_next[j << 1]
                                      + srcv_next[ (j << 1) + 1]
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
        }
        else {
            if (pitchUV && pitchUV < pitch) { // probably YV16
                // TODO
            }
            else if (pitchUV) // YV24
                blit444 (img, dataY, dataU, dataV, pitch);
            else { // Y8
                while (img) {
                    unsigned char y = rgba2y (img->color);
                    unsigned char opacity = 255 - _a (img->color);

                    int i, j;

                    unsigned char *src = img->bitmap;
                    unsigned char *dsty =
                        dataY + img->dst_x + img->dst_y * pitch;

                    if (img->w == 0 || img->h == 0) {
                        img = img->next;
                        continue;
                    }

                    for (i = 0; i < img->h; ++i) {
                        for (j = 0; j < img->w; ++j) {
                            unsigned int k = (src[j] * opacity + 255) >> 8;
                            dsty[j] = (k * y + (255 - k) * dsty[j] + 255) >> 8;
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

void AVSC_CC assrender_destroy (void *ud, AVS_ScriptEnvironment *env) {
    ass_renderer_done ( ( (udata *) ud)->ass_renderer);
    ass_library_done ( ( (udata *) ud)->ass_library);
    ass_free_track ( ( (udata *) ud)->ass);
    free ( ( (udata *) ud)->uv_tmp[0]);
    free ( ( (udata *) ud)->uv_tmp[1]);
    free ( ( (udata *) ud)->lbounds);

    if ( ( (udata *) ud)->isvfr)
        free ( ( (udata *) ud)->timestamp);

    free (ud);
}

AVS_Value AVSC_CC assrender_create (AVS_ScriptEnvironment *env, AVS_Value args,
                                    void *ud) {
    AVS_Value v;
    AVS_FilterInfo *fi;
    AVS_Clip *c = avs_new_c_filter (env, &fi, avs_array_elt (args, 0), 1);
    char e[250];

    const char *f = avs_as_string (avs_array_elt (args, 1));
    const char *vfr = avs_as_string (avs_array_elt (args, 2));
    int h = avs_is_int (avs_array_elt (args, 3)) ?
            avs_as_int (avs_array_elt (args, 3)) : 3;
    double scale = avs_is_float (avs_array_elt (args, 4)) ?
                   avs_as_float (avs_array_elt (args, 4)) : 1.0;
    double line_spacing = avs_is_float (avs_array_elt (args, 5)) ?
                          avs_as_float (avs_array_elt (args, 5)) : 0;
    double dar = avs_is_float (avs_array_elt (args, 6)) ?
                 avs_as_float (avs_array_elt (args, 6)) : 0;
    double sar = avs_is_float (avs_array_elt (args, 7)) ?
                 avs_as_float (avs_array_elt (args, 7)) : 0;
    int top = avs_is_int (avs_array_elt (args, 8)) ?
              avs_as_int (avs_array_elt (args, 8)) : 0;
    int bottom = avs_is_int (avs_array_elt (args, 9)) ?
                 avs_as_int (avs_array_elt (args, 9)) : 0;
    int left = avs_is_int (avs_array_elt (args, 10)) ?
               avs_as_int (avs_array_elt (args, 10)) : 0;
    int right = avs_is_int (avs_array_elt (args, 11)) ?
                avs_as_int (avs_array_elt (args, 11)) : 0;
    const char *cs = avs_as_string (avs_array_elt (args, 12)) ?
                     avs_as_string (avs_array_elt (args, 12)) : "UTF-8";
    int debuglevel = avs_is_int (avs_array_elt (args, 13)) ?
                     avs_as_int (avs_array_elt (args, 13)) : 0;
    const char *fontdir = avs_as_string (avs_array_elt (args, 14)) ?
                          avs_as_string (avs_array_elt (args, 14)) : "";
    const char *srt_font = avs_as_string (avs_array_elt (args, 15)) ?
                           avs_as_string (avs_array_elt (args, 15)) : "Sans";

    ASS_Hinting hinting;
    udata *data;
    ASS_Track *ass;

    if (!avs_is_rgb32 (&fi->vi) && !avs_is_rgb24 (&fi->vi) &&
            !avs_is_yv12 (&fi->vi) && !avs_is_yuv (&fi->vi)) {
        v = avs_new_value_error (
                "AssRender: supported colorspaces: RGB32, RGB24, "
                "YV24, YV16 (TODO), YV12, Y8");
        avs_release_clip (c);
        return v;
    }

    if (!f) {
        v = avs_new_value_error ("AssRender: no input file specified");
        avs_release_clip (c);
        return v;
    }

    switch (h) {
    case 0:
        hinting = ASS_HINTING_NONE;
        break;
    case 1:
        hinting = ASS_HINTING_LIGHT;
        break;
    case 2:
        hinting = ASS_HINTING_NORMAL;
        break;
    case 3:
        hinting = ASS_HINTING_NATIVE;
        break;
    default:
        v = avs_new_value_error ("AssRender: invalid hinting mode");
        avs_release_clip (c);
        return v;
    }

    data = malloc (sizeof (udata));

    if (!init_ass (fi->vi.width, fi->vi.height, scale, line_spacing, dar, sar,
                   top, bottom, left, right, hinting, debuglevel, fontdir,
                   data)) {
        v = avs_new_value_error ("AssRender: failed to initialize");
        avs_release_clip (c);
        return v;
    }

    if (!strcasecmp (strrchr (f, '.'), ".srt"))
        ass = parse_srt (f, data, srt_font);
    else
        ass = ass_read_file (data->ass_library, (char *) f, (char *) cs);

    if (!ass) {
        sprintf (e, "AssRender: unable to parse '%s'", f);
        v = avs_new_value_error (e);
        avs_release_clip (c);
        return v;
    }

    data->ass = ass;

    if (vfr) {
        int ver;
        FILE *fh = fopen (vfr, "r");

        if (!fh) {
            sprintf (e, "AssRender: could not read timecodes file '%s'", vfr);
            v = avs_new_value_error (e);
            avs_release_clip (c);
            return v;
        }

        data->isvfr = 1;

        if (fscanf (fh, "# timecode format v%d", &ver) != 1) {
            sprintf (e, "AssRender: invalid timecodes file '%s'", vfr);
            v = avs_new_value_error (e);
            avs_release_clip (c);
            return v;
        }

        switch (ver) {
        case 1:

            if (!parse_timecodesv1 (fh, fi->vi.num_frames, data)) {
                v = avs_new_value_error (
                        "AssRender: error parsing timecodes file");
                avs_release_clip (c);
                return v;
            }

            break;
        case 2:

            if (!parse_timecodesv2 (fh, fi->vi.num_frames, data)) {
                v = avs_new_value_error (
                        "AssRender: timecodes file had less frames than "
                        "expected");
                avs_release_clip (c);
                return v;
            }

            break;
        }
    }
    else {
        data->isvfr = 0;
    }

    data->uv_tmp[0] = malloc (fi->vi.width * fi->vi.height);
    data->uv_tmp[1] = malloc (fi->vi.width * fi->vi.height);
    data->lbounds = malloc ( ( (fi->vi.height + 1) >> 1)
                             * sizeof ( ( (udata *) ud)->lbounds));

    fi->user_data = data;

    fi->get_frame = assrender_get_frame;

    v = avs_new_value_clip (c);
    avs_release_clip (c);

    avs_at_exit (env, assrender_destroy, data);

    return v;
}

const char *AVSC_CC avisynth_c_plugin_init (AVS_ScriptEnvironment *env) {
    avs_add_function (env, "assrender",
                      "c[file]s[vfr]s[hinting]i[scale]f[line_spacing]f[dar]f"
                      "[sar]f[top]i[bottom]i[left]i[right]i[charset]s"
                      "[debuglevel]i[fontdir]s[srt_font]s",
                      assrender_create, 0);
    return "AssRender 0.22: draws .asses better and faster than ever before";
}

// kate: indent-mode cstyle; space-indent on; indent-width 4; replace-tabs on; 
