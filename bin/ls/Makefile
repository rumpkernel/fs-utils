#	$NetBSD: Makefile,v 1.1 2009/03/23 21:03:58 stacktic Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=	ls
SRCS=	cmp.c ls.c main.c print.c util.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
