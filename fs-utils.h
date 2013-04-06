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

#include <sys/types.h>

#endif /* FSUTILS_H_ */
