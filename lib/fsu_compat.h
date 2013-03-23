#ifndef _FSU_COMPAT_H_
#define _FSU_COMPAT_H_

#include "nbstdlib.h"
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

#endif
