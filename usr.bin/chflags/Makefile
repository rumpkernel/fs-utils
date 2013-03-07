#	$NetBSD: Makefile,v 1.1 2009/03/23 21:03:59 stacktic Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

WARNS=	3
PROG=	chflags
SRCS=	chflags.c

LDADD+= -lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
