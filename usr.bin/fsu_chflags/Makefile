#       $NetBSD: Makefile,v 1.5 2009/11/06 11:47:41 stacktic Exp $
#       @(#)Makefile    8.1 (Berkeley) 6/2/93

.include	"${.CURDIR}/../../Makefile.fsu"

PROG=   	fsu_chflags
.PATH:		../chflags
SRCS=   	chflags.c
MAN=		fsu_chflags.1

OSNAME!=	uname -s
.if ${OSNAME} != "Linux"
LDFLAGS+= 	-Wl,--wrap=chflags,--wrap=lchflags
.endif

.if ${OSNAME} != "NetBSD"
.PATH:	../../fsu_compat
SRCS+=	stat_flags.c
.if ${OSNAME} == "Linux"
NOGCCERROR=1
WARNS=0
.endif
.endif

.include <bsd.prog.mk>
