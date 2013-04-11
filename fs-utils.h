#ifndef FSUTILS_H_
#define FSUTILS_H_

#include "config.h"

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#ifndef __BEGIN_DECLS
# ifdef __cplusplus
#  define __BEGIN_DECLS extern "C" {
#  define __END_DECLS }
# else
#  define __BEGIN_DECLS
#  define __END_DECLS
# endif
#endif

#ifndef __dead
#define __dead __attribute__((noreturn))
#endif

#include <sys/types.h>
#include <stdint.h>

/* compat funcs */

#ifndef HAVE_SETMODE
void *setmode(const char *);
mode_t getmode(const void *, mode_t);
#endif

#ifndef HAVE_STRMODE
void strmode(mode_t, char *);
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
size_t strlcat(char *, const char *, size_t);
#endif

#ifndef HAVE_STRVIS
char *vis(char *, int, int, int);
int strvis(char *, const char *, int);
int strvisx(char *, const char *, size_t, int);
int strnvis(char *, const char *, size_t, int);
#endif

#ifdef HAVE_ERR_H
#include <err.h>
#else
void	err(int, const char *, ...);
void	errx(int, const char *, ...);
void	warn(const char *, ...);
void	warnx(const char *, ...);
#endif

#ifndef HAVE_STRSEP
char *strsep(char **, const char *);
#endif

#ifndef HAVE_U_INT16_T
typedef uint16_t u_int16_t;
#endif
#ifndef HAVE_U_INT32_T
typedef uint32_t u_int32_t;
#endif

#endif /* FSUTILS_H_ */
