#       $NetBSD: Makefile,v 1.4 2009/11/06 11:47:41 stacktic Exp $
#       @(#)Makefile    8.1 (Berkeley) 6/2/93


PROG=   	fsu_chmod
.PATH:		../chmod
SRCS=   	chmod.c
MAN=		fsu_chmod.1

DPADD+= 	${LIBUTIL}

LDFLAGS+= 	-Wl,--wrap=chmod,--wrap=lchmod

.include <bsd.prog.mk>
