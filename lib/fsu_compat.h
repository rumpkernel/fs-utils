#ifndef _FSU_COMPAT_H_
#define _FSU_COMPAT_H_

#include "nbsysstat.h"

#ifndef howmany
#define howmany(x, y) (((x)+((y)-1))/(y))
#endif

#ifndef __UNCONST
#define __UNCONST(a) ((char *)(unsigned long)(const char *)(a))
#endif

int	string_to_flags(char **, unsigned long *, unsigned long *);
char    *flags_to_string(unsigned long, const char *);
int 	humanize_number(char *, size_t, int64_t, const char *, int, int);
char 	*getbsize(int *, long *);
char	*strspct(char *, size_t, int64_t, int64_t, size_t);
char	*strpct(char *, size_t, uint64_t, uint64_t, size_t);

#ifndef HAVE_STRVIS
#undef VIS_OCTAL
#undef VIS_CSTYLE
#undef VIS_SP
#undef VIS_TAB
#undef VIS_NL
#undef VIS_WHITE
#undef VIS_SAFE
#undef VIS_NOSLASH
#undef VIS_HTTPSTYLE
#undef VIS_GLOB

#define VIS_OCTAL       0x01
#define VIS_CSTYLE      0x02
#define VIS_SP          0x04
#define VIS_TAB         0x08
#define VIS_NL          0x10
#define VIS_WHITE       (VIS_SP | VIS_TAB | VIS_NL)
#define VIS_SAFE        0x20
#define VIS_NOSLASH     0x40
#define VIS_HTTPSTYLE   0x80
#define VIS_GLOB        0x100
#endif

#ifndef HAVE_HUMANIZE_NUMBER
#undef HN_DECIMAL
#undef HN_NOSPACE
#undef HN_B
#undef HN_DIVISOR_1000
#undef HN_GETSCALE
#undef HN_AUTOSCALE

#define	HN_DECIMAL		0x01
#define	HN_NOSPACE		0x02
#define	HN_B			0x04
#define	HN_DIVISOR_1000		0x08
#define	HN_GETSCALE		0x10
#define	HN_AUTOSCALE		0x20
#endif

#endif
