#       $NetBSD: Makefile,v 1.4 2009/11/06 11:47:41 stacktic Exp $
#       @(#)Makefile    8.1 (Berkeley) 6/2/93


PROG=   	fsu_ln
.PATH:		../ln
SRCS=   	ln.c
MAN=		fsu_ln.1

LDFLAGS+=        -Wl,--wrap=link,--wrap=symlink

.include <bsd.prog.mk>
