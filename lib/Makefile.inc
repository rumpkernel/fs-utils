#	$NetBSD: Makefile.inc,v 1.4 2009/11/06 11:47:41 stacktic Exp $

.include "../../Makefile.gen"

CFLAGS+=        -I${RUMPINCSDIR}
LDADD+=         -L${RUMPLIBDIR}
