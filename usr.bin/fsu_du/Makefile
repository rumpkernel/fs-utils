#       $NetBSD: Makefile,v 1.4 2009/11/06 11:47:42 stacktic Exp $
#       @(#)Makefile    8.1 (Berkeley) 6/2/93

.include	"${.CURDIR}/../../Makefile.fsu"

PROG=   	fsu_du
.PATH:		../du
SRCS=   	du.c
MAN=		fsu_du.1

OSNAME!=	uname -s
.if ${OSNAME} != "NetBSD"
WARNS=		0
SRCS+=		humanize_number.c getbsize.c
.PATH:		../../fsu_compat
.endif

.include <bsd.prog.mk>
