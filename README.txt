Usage:
assrender(clip, string file, [string vfr, int hinting, float scale,
        float line_spacing, float dar, float sar, int top, int bottom, int left,
        int right, string charset, int verbosity, string fontdir,
        string srt_font])

string file:
    Your subtitle file. May be ASS, SSA or SRT.
string vfr:
    Specify timecodes v1 or v2 file when working with VFRaC.
int hinting:
    Font hinting mode. Choose between 
    none (0), light (1), normal (2) and Freetype native (3, default) autohinting.
float scale:
    Font scale. Defaults to 1.0.
float line_spacing:
    Line spacing in pixels. Defaults to 1.0 and won’t be scaled with frame size.
float dar, float sar:
    Aspect ratio. Of course you need to set both parameters.
int top, int bottom, int left, int right:
    Margins. They will be added to the frame size and may be negative.
string charset:
    Character set to use, in GNU iconv or enca format. Defaults to UTF-8.
    Example enca format: enca:pl:cp1250
        (guess the encoding for Polish, fall back on cp1250)
int verbosity:
    How much crap assrender is supposed to spam to stderr.
string fontdir:
    Additional font directory.
    Useful if you are lazy but want to keep your system fonts clean.
string srt_font:
    Font to use for SRT subtitles.
    Defaults to whatever Fontconfig chooses for “Sans”.