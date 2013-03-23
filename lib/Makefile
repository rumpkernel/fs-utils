TOP=..
include ../Makefile.inc

OBJS=		fsu_mount.o fsu_alias.o \
		mount_cd9660.o mount_ext2fs.o mount_hfs.o  \
		mount_msdos.o mount_tmpfs.o mount_efs.o mount_ffs.o \
		mount_lfs.o mount_ntfs.o mount_udf.o pathadj.o fattr.o \
		mount_sysvbfs.o mount_kernfs.o mount_v7fs.o getmntopts.o \
		fsu_fts.o fsu_dir.o fsu_file.o fsu_str2arg.o \
		getbsize.o stat_flags.o

CFLAGS+=	-DMOUNT_NOMAIN -DUSE_RUMP -I${RUMPBASE}/include -I.
CFLAGS+=-D'__COPYRIGHT(x)=' -D'__RCSID(x)='

# NFS
#OBJS+=		mount_nfs.o
#CFLAGS+=	-DNFS -DWITH_NFS -I${.CURDIR}/external
#.if ${OSNAME} != "NetBSD"
#CFLAGS+=	-I${.CURDIR}/external
#OBJS+=		snprintb.o
#OBJS+=		udp_xfer.o
#OBJS+=		rpc.o net.o
#OBJS+=		getnfsargs_small.o
#CFLAGS+= 	-DNO_PMAP_CACHE

.c.o:
	cc -fPIC -DPIC ${CFLAGS} ${COPTS} -c $<

all: libfsu.so

libfsu.so: ${OBJS}
	cc -o libfsu.so -shared ${OBJS}

clean:
	rm -f libfsu.so *.o
