# $NetBSD: Makefile,v 1.3 2009/11/05 14:02:43 stacktic Exp $
#
.include "../Makefile.inc"

WARNS?=		4

LIB=		fsu_utils

INCS=		fsu_utils.h fsu_fts.h fts2fsufts.h
SRCS=		fsu_fts.c fsu_dir.c fsu_file.c fsu_str2arg.c

CFLAGS+=	-I${.CURDIR}

MAN=		fsu_fclose.3 fsu_ferror.3 fsu_fflush.3 fsu_fgetc.3 fsu_fopen.3 \
		fsu_fputc.3 fsu_fread.3 fsu_fseek.3 fsu_fts.3 fsu_utils.3

MLINKS+=	fsu_ferror.3 fsu_clearerr.3 \
		fsu_ferror.3 fsu_feof.3 \
		fsu_fread.3 fsu_fwrite.3 \
		fsu_fseek.3 fsu_fseeko.3 \
		fsu_fseek.3 fsu_ftell.3 \
		fsu_fseek.3 fsu_ftello.3 \
		fsu_fseek.3 fsu_rewind.3 \
		fsu_fts.3 fsu_fts_open.3 \
		fsu_fts.3 fsu_fts_read.3 \
		fsu_fts.3 fsu_fts_children.3 \
		fsu_fts.3 fsu_fts_set.3 \
		fsu_fts.3 fsu_fts_close.3

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

.include <bsd.lib.mk>
