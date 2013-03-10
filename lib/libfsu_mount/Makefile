#	$NetBSD: Makefile,v 1.8 2009/12/03 15:00:38 pooka Exp $
#
.include 	"../Makefile.inc"

.include	<bsd.own.mk>

LIB=		fsu_mount

SRCS=		fsu_mount.c fsu_alias.c \
		mount_cd9660.c mount_ext2fs.c mount_hfs.c  \
		mount_msdos.c mount_tmpfs.c mount_efs.c mount_ffs.c \
		mount_lfs.c mount_ntfs.c mount_udf.c pathadj.c fattr.c \
		mount_sysvbfs.c mount_kernfs.c mount_v7fs.c

INCS=		fsu_mount.h
MAN=		fsu_mount.3

CFLAGS+=	-I../libfsu_utils -I${.CURDIR}
LDADD+=		-L../libfsu_utils -lfsu_utils

OSNAME!=	uname -s

.if ${OSNAME} != "NetBSD"
SRCS+=		getmntopts.c
.endif
CFLAGS+=	-DHAVE_UKFS_DISKLABEL -DMOUNT_NOMAIN -DUSE_RUMP

SHLIB_MAJOR=    0
SHLIB_MINOR=    0

.ifdef FSU_WITH_SYSPUFFS
WARNS=		0
SRCS+=		rump_syspuffs.c
CFLAGS+= 	-DWITH_SYSPUFFS

.if ${OSNAME} == "NetBSD"
CFLAGS+=	-DHAVE_RUMP_PUB
.endif # NetBSD
.endif # SYSPUFFS

.ifdef ENABLE_NFS
# NFS
SRCS+=		mount_nfs.c
CFLAGS+=	-DNFS -DWITH_NFS -I${.CURDIR}/external
.if ${OSNAME} != "NetBSD"
CFLAGS+=	-I${.CURDIR}/external
SRCS+=		snprintb.c
.endif

SRCS+=		udp_xfer.c
SRCS+=		rpc.c net.c
SRCS+=		getnfsargs_small.c
CFLAGS+= 	-DNO_PMAP_CACHE
.endif

.ifdef FSU_WITH_SMBFS
.PATH:		${NETBSDSRCDIR}/dist/smbfs/lib/smb
SRCS+=		mount_smbfs.c \
		rcfile.c ctx.c cfopt.c subr.c nls.c rap.c mbuf.c rq.c file.c \
		print.c kiconv.c nb.c nb_name.c nb_net.c nbns_rq.c

CFLAGS+=	-DSMBFS -Wno-pointer-sign -DWITH_SMBFS
CFLAGS+=	-I${NETBSDSRCDIR}/dist/smbfs/include -I${NETBSDSRCDIR}/sys
CFLAGS+=	-I${NETBSDSRCDIR}/sys/fs/smbfs
.endif # SMBFS

.include <bsd.lib.mk>
