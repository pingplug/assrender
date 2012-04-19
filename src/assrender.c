#include "avisynth_c.h"
#include <ass/ass.h>
#include <fontconfig/fontconfig.h>
#include <stdlib.h>
#include <math.h>

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

ASS_Library *ass_library;
ASS_Renderer *ass_renderer;
ASS_Track *ass;

int64_t *timestamp;
int isvfr = 0;

int parse_timecodesv1(FILE *f, int total) {
    // we generate our timecodes for all frames and put them into an array
    int start, end, n = 0;
    double t = 0.0, basefps = 0.0, fps;
    int64_t ts[total];
    char l[1025];

    while ((fgets(l, 1024, f) != NULL) && n < total) {
        if (l[0] == 0 || l[0] == '\n' || l[0] == '\r' || l[0] == '#')
            continue;

        if (sscanf(l, "Assume %lf", &basefps) == 1)
            continue;

        if (!sscanf(l, "%d,%d,%lf", &start, &end, &fps) == 3)
            continue;

        if (basefps == 0.0)
            continue;

        while (n < start) {
            ts[n++] = (int64_t)rint(t);
            t += 1000.0/basefps;
        }

        while (n <= end) {
            ts[n++] = (int64_t)rint(t);
            t += 1000.0/fps;
        }
    }

    fclose(f);

    if (basefps == 0.0)
        return 0;

    while (n < total) {
        ts[n++] = (int64_t)rint(t);
        t += 1000.0/basefps;
    }

    timestamp = ts;
    return 1;
}

int parse_timecodesv2(FILE *f, int total) {
    int n = 0;
    int64_t ts[total];
    char l[1025];

    while ((fgets(l, 1024, f) != NULL) && n < total) {
        if (l[0] == 0 || l[0] == '\n' || l[0] == '\r' || l[0] == '#')
            continue;

        // naive but works unless someone is fucking with us
        ts[n++] = (int64_t)atoi(l);
    }

    fclose(f);

    if (n < total)
        return 0;

    timestamp = ts;
    return 1;
}

void msg_callback(int level, const char *fmt, va_list va, void *data) {
    if (level > (int)data)
        return;
    printf("libass: ");
    vprintf(fmt, va);
    printf("\n");
}

static int init_ass(int w, int h, double scale, ASS_Hinting hinting,
                    int verbosity) {
    ass_library = ass_library_init();
    if (!ass_library)
        return 0;

    ass_set_message_cb(ass_library, msg_callback, (void*)verbosity);

    ass_set_extract_fonts(ass_library, 0);
    ass_set_style_overrides(ass_library, 0);

    ass_renderer = ass_renderer_init(ass_library);
    if (!ass_renderer)
        return 0;

//     manually create/update Fontconfig’s cache, and be verbose
//     extern int FcDebugVal;
//     FcDebugVal = 128;
//     FcConfigEnableHome(FcFalse);
//     FcInit();
//     FcDebugVal = 0;

    ass_set_font_scale(ass_renderer, scale);
    ass_set_hinting(ass_renderer, hinting);

    ass_set_frame_size(ass_renderer, w, h);
    ass_set_fonts(ass_renderer, "Arial", "Sans", 1, NULL, 1);

    return 1;
}

AVS_VideoFrame *AVSC_CC assrender_get_frame(AVS_FilterInfo * p, int n) {
    AVS_VideoFrame *src;
    int row_size, height, pitch;
    BYTE *data;
    int x, y;

    src = avs_get_frame(p->child, n);

    avs_make_writable(p->env, &src);
    data = avs_get_write_ptr(src);
    pitch = avs_get_pitch(src);
    row_size = avs_get_row_size(src);
    height = avs_get_height(src);

    int64_t ts;
    if (isvfr < 1) {
        // it’s a casting party!
        ts = (int64_t)n * (int64_t)1000 * (int64_t)p->vi.fps_denominator /
             (int64_t)p->vi.fps_numerator;
    }
    else {
        ts = timestamp[n];
    }

    ASS_Image *img = ass_render_frame(ass_renderer, ass, ts, NULL);

    while (img) {
        if (img->w == 0 || img->h == 0)
            continue;

        int x, y;
        unsigned char a = 255 - _a(img->color);
        unsigned char r = _r(img->color);
        unsigned char g = _g(img->color);
        unsigned char b = _b(img->color);

        unsigned char *src;
        unsigned char *dst;
        unsigned char k, ck, t;
        int dst_delta;

        src = img->bitmap;
        dst = data + (pitch * (height - img->dst_y - 1)) + img->dst_x * 4;
        dst_delta = pitch + img->w * 4;
        // Move destination pointer to the bottom right corner of the bounding
        // box that contains the current overlay bitmap.
        // Remember that avisynth rgb32 bitmaps are upside down, hence we need
        // to render upside down.

        for (y = 0; y < img->h; y++) {
            for (x = 0; x < img->w; ++x) {
                k = ((unsigned)src[x]) * a / 255;
                ck = 255 - k;
                t = *dst;
                *dst++ = (k*b + ck*t) / 255;
                t = *dst;
                *dst++ = (k*g + ck*t) / 255;
                t = *dst;
                *dst++ = (k*r + ck*t) / 255;
                dst++;
            }

            dst -= dst_delta;   // back up a scanline (rendering bottom-to-top)
            src += img->stride; // advance source one scanline
        }

        img = img->next;
    }
    return src;
}

AVS_Value AVSC_CC assrender_create(AVS_ScriptEnvironment *env, AVS_Value args,
                                   void *dg) {
    AVS_Value v;
    AVS_FilterInfo * fi;
    AVS_Clip *c = avs_new_c_filter(env, &fi, avs_array_elt(args, 0), 1);
    char e[250];
    if (!avs_is_rgb32(&fi->vi)) {
        v = avs_new_value_error("AssRender: Only RGB32 is supported!");
        avs_release_clip(c);
        return v;
    }

    const char *f = avs_as_string(avs_array_elt(args, 1));
    int h = avs_is_int(avs_array_elt(args, 2)) ?
            avs_as_int(avs_array_elt(args, 2)) : 3;
    double scale = avs_is_float(avs_array_elt(args, 3)) ?
                   avs_as_float(avs_array_elt(args, 3)) : 1.0;
    const char *cs = avs_as_string(avs_array_elt(args, 4)) ?
                     avs_as_string(avs_array_elt(args, 4)) : "UTF-8";
    int verbosity = avs_is_int(avs_array_elt(args, 5)) ?
                    avs_as_int(avs_array_elt(args, 5)) : 0;
    const char *vfr = avs_as_string(avs_array_elt(args, 6));
    ASS_Hinting hinting;

    if (!f) {
        v = avs_new_value_error("AssRender: no input file specified");
        avs_release_clip(c);
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
        v = avs_new_value_error("AssRender: invalid hinting mode");
        avs_release_clip(c);
        return v;
    }

    if (!init_ass(fi->vi.width, fi->vi.height, scale, hinting, verbosity)) {
        v = avs_new_value_error("AssRender: failed to initialize");
        avs_release_clip(c);
        return v;
    }

    ass = ass_read_file(ass_library, f, cs);
    if (!ass) {
        sprintf(e, "AssRender: could not read '%s'", f);
        v = avs_new_value_error(e);
        avs_release_clip(c);
        return v;
    }

    if (vfr) {
        FILE *fh = fopen(vfr, "r");
        if (!fh) {
            sprintf(e, "AssRender: could not read timecodes file '%s'", vfr);
            v = avs_new_value_error(e);
            avs_release_clip(c);
            return v;
        }
        isvfr = 1;
        int ver;
        if (fscanf(fh, "# timecode format v%d", &ver) != 1) {
            sprintf(e, "AssRender: invalid timecodes file '%s'", vfr);
            v = avs_new_value_error(e);
            avs_release_clip(c);
            return v;
        }
        switch (ver) {
        case 1:
            if (!parse_timecodesv1(fh, fi->vi.num_frames)) {
                v = avs_new_value_error(
                        "AssRender: error parsing timecodes file");
                avs_release_clip(c);
                return v;
            }
            break;
        case 2:
            if (!parse_timecodesv2(fh, fi->vi.num_frames)) {
                v = avs_new_value_error(
                        "AssRender: timecodes file had less frames than expected");
                avs_release_clip(c);
                return v;
            }
            break;
        }
    }

    fi->get_frame = assrender_get_frame;
    v = avs_new_value_clip(c);
    avs_release_clip(c);

    return v;
}

const char *AVSC_CC avisynth_c_plugin_init(AVS_ScriptEnvironment *env) {
    avs_add_function(env, "assrender",
                     "c[file]s[hinting]i[scale]f[charset]s[verbosity]i[vfr]s",
                     assrender_create, 0);
    return "AssRender 0.16: draws .asses better and faster than ever before";
}

