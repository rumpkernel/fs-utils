TOP=${CURDIR}/..
include ../Makefile.inc

ALLPROGS= fsu_cat fsu_chmod fsu_cp fsu_diff fsu_dir fsu_ecp fsu_exec fsu_find fsu_ln fsu_ls fsu_mkdir fsu_mv fsu_rm fsu_rmdir fsu_stat fsu_write fsu_mknod fsu_chflags fsu_du fsu_mkfifo fsu_touch fsu_chown

CPPFLAGS+=-I${RUMPBASE}/include -I${TOP}/lib -DUSE_RUMP
CPPFLAGS+=-D'__COPYRIGHT(x)=' -D'__RCSID(x)='
CPPFLAGS+=-D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=500 -D_BSD_SOURCE
LDFLAGS=-L${TOP}/lib -Wl,-R${TOP}/lib -L${RUMPBASE}/lib -Wl,-R${RUMPBASE}/lib
OSNAME=	$(shell uname -s)
ifeq ($(OSNAME),Linux)
LIBS= -lrumpnet_sockin -lrumpnet_net -lrumpnet -lrumpkern_sys_linux
endif
LIBS+= -lfsu -lrumpvfs -lrumpdev_disk -lrumpdev -lrump -lrumpuser
ifeq ($(OSNAME),Linux)
LIBS+= -lbsd -ldl -lpthread -lutil -lc
endif
LIBS+= -lpthread -lutil -lc

all: ${ALLPROGS}

.c.o:
	cc -c $< ${CPPFLAGS} ${COPTS}

fsu_cat: fsu_cat.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_chflags: chflags.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_chmod: chmod.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS} -Wl,--wrap=chmod,--wrap=lchmod

fsu_chown: chown.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS} -Wl,--wrap=chown,--wrap=lchown

fsu_cp: cp.o utils_cp.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_diff: fsu_diff.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_dir: fsu_dir.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_du: du.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_ecp: fsu_ecp.o fsu_flist.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_exec: fsu_exec.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_find: fsu_find.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_ln: ln.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS} -Wl,--wrap=link,--wrap=symlink

fsu_ls: cmp.o ls.o main.o print.o utils_ls.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_mkdir: mkdir.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_mkfifo: mkfifo.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_mknod: mknod.o pack_dev.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_mv: fsu_mv.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_rm: rm.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_rmdir: rmdir.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_stat: fsu_stat.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_touch: fsu_touch.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

fsu_write: fsu_write.o
	cc -o $@ $^ ${LDFLAGS} ${LIBS}

clean:
	rm -f ${ALLPROGS} *.o .dummy
