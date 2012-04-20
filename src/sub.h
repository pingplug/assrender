void ass_read_colorspace(const char *f, char *csp);

ASS_Track *parse_srt(const char *f, udata *ud, const char *srt_font);

void msg_callback(int level, const char *fmt, va_list va, void *data);

int init_ass(int w, int h, double scale, double line_spacing,
             ASS_Hinting hinting, double dar, double sar, int top,
             int bottom, int left, int right, int verbosity,
             const char *fontdir, udata *ud);
