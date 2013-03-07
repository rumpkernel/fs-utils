#       $NetBSD: Makefile,v 1.3 2009/11/05 14:39:15 stacktic Exp $
#       @(#)Makefile    8.1 (Berkeley) 6/2/93


PROG=   fsu_ls
.PATH:	../ls
SRCS=   cmp.c ls.c main.c print.c util.c

OSNAME!=	uname -s
.if ${OSNAME} != "NetBSD"
.PATH:	../../fsu_compat
SRCS+=	stat_flags.c humanize_number.c getbsize.c
.endif

MAN=	fsu_ls.1

.include <bsd.prog.mk>
