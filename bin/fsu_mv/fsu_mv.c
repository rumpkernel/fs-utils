/* $NetBSD: fsu_mv.c,v 1.2 2009/11/05 14:39:15 stacktic Exp $ */

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

#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rump/rump_syscalls.h>

#include <fsu_mount.h>

#define FSU_MV_INTERACTIVE (0x01)
#define FSU_MV_VERBOSE (FSU_MV_INTERACTIVE<<1)



static int fsu_mv_parse_arg(int *, char ***);
static int fsu_mv(const char *, const char *, int);
static int is_user_ok(const char *);
static int move_to_dir(const char *, const char *, int);
static int move_to_file(const char *, const char *, int);
static void usage(void);

int main(int, char *[]);

int
main(int argc, char *argv[])
{
	int cur_arg, flags = 0, rv;

	setprogname(argv[0]);

	if (fsu_mount(&argc, &argv) != 0)
		errx(-1, NULL);

	flags = fsu_mv_parse_arg(&argc, &argv);
	if (flags == -1 || argc < 2) {
		usage();
		return -1;
	}

	for (rv = 0, cur_arg = 0; cur_arg < argc-1; cur_arg++)
		rv |= fsu_mv(argv[cur_arg], argv[argc-1], flags);

	return rv;
}

static int
fsu_mv_parse_arg(int *argc, char ***argv)
{
	int flags, rv;

	flags = 0;
	while ((rv = getopt(*argc, *argv, "iv")) != -1) {
		switch (rv) {
		case 'i':
			flags |= FSU_MV_INTERACTIVE;
			break;
		case 'v':
			flags |= FSU_MV_VERBOSE;
			break;
		case '?':
		default:
			return -1;
		}
	}
	*argc -= optind;
	*argv += optind;

	return flags;
}


static int
fsu_mv(const char *from, const char *to, int flags)
{
	int rv;
	struct stat file_stat;

	rv = rump_sys_stat(to, &file_stat);
	if (rv == -1 || !S_ISDIR(file_stat.st_mode))
		return move_to_file(from, to, flags);

	return move_to_dir(from, to, flags);
}

static int
is_user_ok(const char *to)
{
	int ch, ch2;

	printf("Overwrite %s? ", to);
	if ((ch = getchar()) != EOF && ch != '\n')
		while ((ch2 = getchar()) != EOF && ch2 != '\n')
			continue;

	if (ch != 'y' && ch != 'Y')
		return 0;
	return 1;
}

static int
move_to_dir(const char *from, const char *to, int flags)
{
	const char *p;
	int rv;
	char path[PATH_MAX + 1];
	struct stat file_stat;
	size_t len;

	len = strlen(to);
	rv = strlcpy(path, to, PATH_MAX + 1);
	if (rv != (int)len) {
		warn("%s", to);
		return -1;
	}

	rv = rump_sys_stat(from, &file_stat);
	if (rv == -1) {
		warn("%s", from);
		return -1;
	}

	p = strrchr(from, '/');
	if (p == NULL)
		p = from;
	else
		++p;

	rv = strlcat(path, p, PATH_MAX + 1);
	if (rv == -1) {
		warn("%s/%s", path, p);
		return -1;
	}

	if (flags & FSU_MV_VERBOSE)
		printf("%s -> %s\n", from, path);

	if (flags & FSU_MV_INTERACTIVE) {
		rv = rump_sys_stat(path, &file_stat);
		if (rv != -1 && !is_user_ok(to))
			return 0;
	}

	rv = rump_sys_rename(from, path);
	if (rv == -1) {
		warn("%s", from);
		return -1;
	}
	return 0;
}

static int
move_to_file(const char *from, const char *to, int flags)
{
	int rv;
	struct stat file_stat;

	if (flags & FSU_MV_VERBOSE)
		printf("%s -> %s\n", from, to);

	if (flags & FSU_MV_INTERACTIVE) {
		rv = rump_sys_stat(to, &file_stat);
		if (rv != -1 && !is_user_ok(to))
			return 0;
	}
	rv = rump_sys_rename(from, to);
	if (rv == -1) {
		warn("%s or %s", from, to);
		return -1;
	}
	return 0;
}

static void
usage(void)
{

	fprintf(stderr, "usage: \t%s %s [-iv] source target\n"
		"       \t%s %s [-iv] source ... directory\n",
		getprogname(), fsu_mount_usage(),
		getprogname(), fsu_mount_usage());

	exit(EXIT_FAILURE);
}
