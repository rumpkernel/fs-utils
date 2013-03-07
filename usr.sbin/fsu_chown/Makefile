#       $NetBSD: Makefile,v 1.4 2009/11/16 07:22:17 stacktic Exp $
#       @(#)Makefile    8.1 (Berkeley) 6/2/93


PROG=   	fsu_chown
.PATH:		../chown
SRCS=   	chown.c
MAN=		fsu_chown.1 fsu_chgrp.1

LDFLAGS+= 	-Wl,--wrap=chown,--wrap=lchown

.include "${.CURDIR}/../../Makefile.fsu"
.include <bsd.prog.mk>
