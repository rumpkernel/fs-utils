/* $NetBSD: fsu_diff.c,v 1.2 2009/11/05 14:39:15 stacktic Exp $ */

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

#include <sys/stat.h>
#ifdef __NetBSD__
#include <sys/syslimits.h>
#elif !defined(PATH_MAX)
#define PATH_MAX (1024)
#endif

#if HAVE_NBCOMPAT_H
#include <nbcompat.h>
#endif

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rump/rump_syscalls.h>

#include <fsu_fts.h>
#include <fsu_utils.h>
#include <fsu_mount.h>



#define BUFSIZE (8192)

static int fsu_diff(const char *p1, const char *p2);
static int fsu_diff_file(const FSU_FTSENT *, const FSU_FTSENT *);
static int fsu_diff_fent(const FSU_FTSENT *, const FSU_FTSENT *);
static int fsu_diff_link(const FSU_FTSENT *, const FSU_FTSENT *);
static void usage(void);
int	   fsu_diff_namecmp(const FSU_FTSENT **, const FSU_FTSENT **);

int
main(int argc, char *argv[])
{
	int rv;

	setprogname(argv[0]);

	if (fsu_mount3(&argc, &argv, MOUNT_READONLY) != 0)
		errx(-1, NULL);

	if (argc != 3) {
		usage();
		/* NOTREACHED */
	}

	rv = fsu_diff(argv[1], argv[2]);

	return rv != 0;
}

static int
fsu_diff(const char *p1, const char *p2)
{
	FSU_FTS *r1, *r2;
	FSU_FTSENT *e1, *e2;
	int rv, diff;
	char *const pa1[] = { strdup(p1), NULL };
	char *const pa2[] = { strdup(p2), NULL };

	if (strcmp(p1, p2) == 0)
		return 0;

	r1 = fsu_fts_open(pa1, FTS_LOGICAL, fsu_diff_namecmp);
	if (r1 == NULL)
		return -1;

	r2 = fsu_fts_open(pa2, FTS_LOGICAL, fsu_diff_namecmp);
	if (r2 == NULL) {
		fsu_fts_close(r1);
		return -1;
	}
	rv = 0;
	for (;;) {
		diff = 0;
		e1 = fsu_fts_read(r1);
		e2 = fsu_fts_read(r2);
	again:
		if (e1 != NULL && e2 != NULL &&
		    S_ISDIR(e1->fts_statp->st_mode) &&
		    e1->fts_level == e2->fts_level &&
		    e1->fts_level == FTS_ROOTLEVEL)
			continue;

		if (e1 != NULL && e1->fts_info == FTS_DP) {
			e1 = fsu_fts_read(r1);
			goto again;
		} else if (e2 != NULL && e2->fts_info == FTS_DP) {
			e2 = fsu_fts_read(r2);
			goto again;
		}

		if (e1 == NULL) {
			if (e2 == NULL)
				break;
			if (e2->fts_level != FTS_ROOTLEVEL)
				fprintf(stderr, "%s: only on %s\n",
					e2->fts_accpath, p2);
			continue;
		} else if (e2 == NULL) {
			if (e1->fts_level != FTS_ROOTLEVEL)
				fprintf(stderr, "%s: only on %s\n",
					e1->fts_accpath, p1);
			continue;
		}

		diff = strcmp(e1->fts_name, e2->fts_name);
		if (diff != 0 &&
		    e1->fts_level != FTS_ROOTLEVEL &&
		    e2->fts_level != FTS_ROOTLEVEL) {
			if (diff < 0) {
				fprintf(stderr, "%s: only on %s\n",
					e1->fts_accpath, p1);
				e1 = fsu_fts_read(r1);
			} else {
				fprintf(stderr, "%s: only on %s\n",
					e2->fts_accpath, p2);
				e2 = fsu_fts_read(r2);
			}
			goto again;
		}

		diff = fsu_diff_fent(e1, e2);
		if (diff != 0)
			fprintf(stderr, "%s and %s differ\n",
				e1->fts_path, e2->fts_path);
		rv |= diff;
	}

	fsu_fts_close(r1);
	fsu_fts_close(r2);
	free(pa1[0]);
	free(pa2[0]);
	return rv == 0 ? 0 : -1;
}

static int
fsu_diff_fent(const FSU_FTSENT *f1, const FSU_FTSENT *f2)
{

	if (f1->fts_statp->st_ino == f2->fts_statp->st_ino)
		return 0;

	if ((f1->fts_statp->st_mode & S_IFMT) !=
	    (f2->fts_statp->st_mode & S_IFMT))
		return -1;

	switch (f1->fts_statp->st_mode & S_IFMT) {
	case S_IFIFO:
		return 0;
	case S_IFCHR:
	case S_IFBLK:
		return (f1->fts_statp->st_rdev != f2->fts_statp->st_rdev);
	case S_IFDIR:
		if (f1->fts_nlink != f2->fts_nlink)
			return -1;
		return strcmp(f1->fts_name, f2->fts_name);
	case S_IFREG:
		return fsu_diff_file(f1, f2);
	case S_IFLNK:
		return fsu_diff_link(f1, f2);
	default:
		return -1;
	}
}

static int
fsu_diff_file(const FSU_FTSENT *f1, const FSU_FTSENT *f2)
{
	uint8_t buf1[BUFSIZE], buf2[BUFSIZE];
	int rd1, rd2, fd1, fd2;
	off_t off, diff;

	if (f1->fts_statp->st_size != f2->fts_statp->st_size)
		return -1;
	if (f1->fts_level != FTS_ROOTLEVEL &&
	    strcmp(f1->fts_name, f2->fts_name) != 0)
		return -1;

	fd1 = rump_sys_open(f1->fts_accpath, O_RDONLY);
	fd2 = rump_sys_open(f2->fts_accpath, O_RDONLY);
	off = 0;
	do {
		rd1 = rump_sys_read(fd1, buf1, BUFSIZE);
		if (rd1 == -1) {
			warn("%s", f1->fts_accpath);
			return -1;
		}

		rd2 = rump_sys_read(fd2, buf2, BUFSIZE);
		if (rd2 == -1) {
			warn("%s", f2->fts_accpath);
			return -1;
		}

		if (rd1 != rd2)
			return 1;

		diff = memcmp(buf1, buf2, rd1);
		if (diff != 0)
			return 1;

		off += rd1;
	} while (rd1 > 0);
	return 0;
}

static int
fsu_diff_link(const FSU_FTSENT *f1, const FSU_FTSENT *f2)
{
	char buf1[PATH_MAX + 1], buf2[PATH_MAX + 1];
	int rd1, rd2;

	if (strcmp(f1->fts_name, f2->fts_name) != 0)
		return -1;

	rd1 = rump_sys_readlink(f1->fts_accpath, buf1, PATH_MAX);
	if (rd1 == -1) {
		warn("%s", f1->fts_accpath);
		return -1;
	}
	rd2 = rump_sys_readlink(f2->fts_accpath, buf2, PATH_MAX);
	if (rd2 == -1) {
		warn("%s", f2->fts_accpath);
		return -1;
	}
	if (rd1 != rd2 || strncmp(buf1, buf2, rd1) != 0)
		return -1;

	return 0;
}

int
fsu_diff_namecmp(const FSU_FTSENT **a, const FSU_FTSENT **b)
{

	return (strcmp((*a)->fts_name, (*b)->fts_name));
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s %s filename1 filename2\n",
		getprogname(), fsu_mount_usage());

	exit(EXIT_FAILURE);
}
