#ifndef _TIMECODE_H_
#define _TIMECODE_H_

#include "assrender.h"

int parse_timecodesv1(FILE *f, int total, udata *ud);

int parse_timecodesv2(FILE *f, int total, udata *ud);

#endif
