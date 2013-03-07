#	$NetBSD: Makefile,v 1.1 2009/04/14 17:37:25 pooka Exp $
#

NOMAN=	1

.include <bsd.own.mk>

PROG=	makefs-analyzetree
SRCS=	ffs.c ext2fs.c \
	getid.c \
	makefs.c misc.c \
	pack_dev.c \
	spec.c \
	walk.c

MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree

CPPFLAGS+=	-I${.CURDIR} -I${MKNODSRC} -I${MTREESRC}
.PATH:		${MKNODSRC} ${MTREESRC}

.if (${HOSTPROG:U} == "")
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

WARNS?=	3
DBG=-g

.include <bsd.prog.mk>
