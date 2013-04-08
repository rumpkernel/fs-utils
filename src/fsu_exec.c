/* $NetBSD: fsu_exec.c,v 1.2 2009/11/05 14:39:15 stacktic Exp $ */

/*
 * Copyright (c) 2008 Arnaud Ysmal.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "fs-utils.h"
#ifdef __NetBSD__
#include <sys/syslimits.h>
#elif !defined(PATH_MAX)
#define PATH_MAX (1024)
#endif
#include <sys/wait.h>
#include <sys/stat.h>

#if HAVE_NBCOMPAT_H
#include <nbcompat.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rump_syscalls.h>

#include <fsu_utils.h>
#include <fsu_mount.h>



static void usage(void);

#define BUFSIZE (8192)

static int copy_file(const char *, const char *, bool);
static void usage(void);

int
main(int argc, char **argv)
{
	char tmpfname[PATH_MAX];
	int len, rv;
	struct stat sb;
	pid_t child;
	char *from;

	setprogname(argv[0]);

	if (fsu_mount(&argc, &argv, MOUNT_READWRITE) != 0)
		errx(-1, NULL);

	if (argc < 3)
		usage();
	++argv;
	--argc;

	from = argv[argc - 1];
	argv[argc - 1] = tmpfname;

	rv = snprintf(tmpfname, PATH_MAX-1, "/tmp/fsutmp.%i", getpid());
	if (rv <= 0) {
		fprintf(stderr, "error in snprintf\n");
		return -1;
	}

	if (stat(tmpfname, &sb) == 0)
		unlink(tmpfname);

	copy_file(from, tmpfname, true);

	child = fork();
	switch (child) {
	case -1:
		warn("fork");
		rv = -1;
		goto out;
	case 0:
		execvp(argv[0], argv);
		exit(EXIT_FAILURE);
	default:
		waitpid(child, &len, 0);
	}

	copy_file(tmpfname, from, false);

out:
	unlink(tmpfname);
	if (WIFEXITED(len))
		return WEXITSTATUS(len);
	return -1;
}

/*
 * get == true -> Copying from the image to the real fs
 * get == false -> Copying from the real fs to the image
 */
static int
copy_file(const char *from, const char *to, bool get)
{
	uint8_t buf[BUFSIZE];
	ssize_t rd, wr;
	off_t off;
	int fd, fd2, rv;
	struct stat from_stat;

	fd = -1;

	if (!get)
		rv = stat(from, &from_stat);
	else
		rv = rump_sys_stat(from, &from_stat);
	if (rv == -1) {
		warn("%s", from);
		return -1;
	}

	if (get) {
		fd = rump_sys_open(from, from_stat.st_mode);
		fd2 = rv = open(to, O_WRONLY|O_CREAT|O_EXCL, 0777);
	} else {
		fd = rv = open(from, O_RDONLY, from_stat.st_mode);
		rump_sys_unlink(to);
		fd2 = rump_sys_open(to, O_WRONLY|O_CREAT|O_EXCL, from_stat.st_mode);
	}
	if (rv == -1) {
		warn("%s", to);
		return -1;
	}

	off = 0;
	do {
		if (!get)
			rd = read(fd, buf, BUFSIZE);
		else
			rd = rump_sys_read(fd, buf, BUFSIZE);
		if (rd == -1) {
			warn("%s", from);
			rv = -1;
			goto out;
		}
		if (get)
			wr = write(fd2, buf, rd);
		else
			wr = rump_sys_write(fd2, buf, rd);
		if (wr == -1 || wr != rd) {
			warn("%s", to);
			rv = -1;
			goto out;
		}
		off += rd;
	} while (rd == BUFSIZE);

	rv = 0;
out:

	if (get) {
		rump_sys_close(fd);
		close(fd2);
	} else {
		close(fd);
		rump_sys_close(fd2);
	}
	return rv;
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s %s command file...\n",
		getprogname(), fsu_mount_usage());

	exit(EXIT_FAILURE);
}
