#define _FILE_OFFSET_BITS 64
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <fsu_mount.h>
struct linux_dirent {
	unsigned long  d_ino;     /* Inode number */
	unsigned long  d_off;     /* Offset to next linux_dirent */
	unsigned short d_reclen;  /* Length of this linux_dirent */
	char d_type;
	char           d_name[];  /* Filename (null-terminated) */
	/* length is actually (d_reclen - 2 -
	   offsetof(struct linux_dirent, d_name) */

};

int
main(int argc, char *argv[])
{
	int fd, nread;
	char buf[4096];
	struct linux_dirent *d;
	struct dirent *dir;
	int bpos;
	char d_type;

	setprogname(argv[0]);
	if (fsu_mount(&argc, &argv) != 0)
		errx(-1, NULL);
	printf("Stating %s\n", argv[1]);

	printf("       dirent - linux_dirent\n");
	printf("d_ino: %d %d - %d %d\n",
			sizeof(dir->d_ino), (uintptr_t)(&((struct dirent *)NULL)->d_ino),
			sizeof(d->d_ino), (uintptr_t)(&((struct linux_dirent *)NULL)->d_ino));
	printf("d_off: %d %d - %d %d\n",
			sizeof(dir->d_off), (uintptr_t)(&((struct dirent *)NULL)->d_off),
			sizeof(d->d_off), (uintptr_t)(&((struct linux_dirent *)NULL)->d_off));
	printf("d_reclen: %d %d - %d %d\n",
			sizeof(dir->d_reclen), (uintptr_t)(&((struct dirent *)NULL)->d_reclen),
			sizeof(d->d_reclen), (uintptr_t)(&((struct linux_dirent *)NULL)->d_reclen));
	printf("d_type: %d %d - %d %d\n",
			sizeof(dir->d_type), (uintptr_t)(&((struct dirent *)NULL)->d_type),
			sizeof(d->d_type), (uintptr_t)(&((struct linux_dirent *)NULL)->d_type));

	printf("d_name(offset): %d - %d\n",
			(uintptr_t)(&((struct dirent *)NULL)->d_name),
			(uintptr_t)(&((struct linux_dirent *)NULL)->d_name));
	fd = rump_sys_open(argc > 1 ? argv[1] : ".", RUMP_O_RDONLY | RUMP_O_DIRECTORY);
	if (fd == -1)
		err(-1, "open");

	while ((nread = rump_sys_getdents(fd, buf, sizeof(buf))) > 0) {
		printf("--------------- nread=%d ---------------\n", nread);
		printf("i-node#   d_reclen  d_off   d_name\n");
		for (bpos = 0; bpos < nread;) {
			d = (struct linux_dirent *) (buf + bpos);
			printf("  %8ld  %4d %10lu  %s\n", d->d_ino, d->d_reclen,  d->d_off, (char *) d->d_name);
			dir = (struct dirent *)(buf + bpos);
			printf("- %8ld  %4d %10lld  %s\n", dir->d_ino, dir->d_reclen,  dir->d_off, (char *) &dir->d_name);
			bpos += d->d_reclen;
		}
	}

	exit(EXIT_SUCCESS);


	return 0;
}
