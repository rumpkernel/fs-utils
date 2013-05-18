/* $NetBSD: fsu_cat.c,v 1.2 2009/11/05 14:39:14 stacktic Exp $ */

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

#include <sys/stat.h>

#if HAVE_NBCOMPAT_H
#include <nbcompat.h>
#endif

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rump.h>
#include <rump/rumpdefs.h>
#include <rump/rump_syscalls.h>

#include <fsu_utils.h>
#include <fsu_mount.h>

#define FSU_CAT_NOT_NUMBER_BLANK (0x01)
#define FSU_CAT_DOLLAR_EOL (FSU_CAT_NOT_NUMBER_BLANK<<1)
#define FSU_CAT_NUMBER (FSU_CAT_DOLLAR_EOL<<1)
#define FSU_CAT_SINGLE_NL (FSU_CAT_NUMBER<<1)
#define FSU_CAT_TAB (FSU_CAT_SINGLE_NL<<1)
#define FSU_CAT_NON_PRINTING (FSU_CAT_TAB<<1)
#define BUFSIZE (8192)

static int	fsu_cat(const char *, int);
static int	fsu_cat_parse_arg(int *, char ***);
static void	fsu_cook_buf(const char *, int);
static int	fsu_raw_cat(const char *);
static void	usage(void);

int
main(int argc, char *argv[])
{
	int cur_arg, flags, rv;

	setprogname(argv[0]);

	if (fsu_mount(&argc, &argv, MOUNT_READONLY) != 0)
		usage();

	flags = fsu_cat_parse_arg(&argc, &argv);
	if (argc < 1)
		usage();

	for (rv = 0, cur_arg = 0; cur_arg < argc; ++cur_arg)
		rv |= fsu_cat(argv[cur_arg], flags);

	return rv;
}

static int
fsu_cat_parse_arg(int *argc, char ***argv)
{
	int flags, rv;

	flags = 0;
	while ((rv = getopt(*argc, *argv, "benstv")) != -1) {
		switch (rv) {
		case 'b':
			/* -b implies -n */
			flags |= FSU_CAT_NOT_NUMBER_BLANK | FSU_CAT_NUMBER;
			break;
		case 'e':
			/* -e implies -v */
			flags |= FSU_CAT_DOLLAR_EOL | FSU_CAT_NON_PRINTING;
			break;
		case 'n':
			flags |= FSU_CAT_NUMBER;
			break;
		case 's':
			flags |= FSU_CAT_SINGLE_NL;
			break;
		case 't':
			/* -t implies -v */
			flags |= FSU_CAT_TAB | FSU_CAT_NON_PRINTING;
			break;
		case 'v':
			flags |= FSU_CAT_NON_PRINTING;
			break;
		default:
		case '?':
			usage();
		}
	}
	*argc -= optind;
	*argv += optind;
	return flags;
}

static int
fsu_cat(const char *filename, int flags)
{
	int rv;
	struct stat file_stat;

	rv = rump_sys_stat(filename, &file_stat);
	if (rv == -1 && !(filename[0] == '-' && filename[1] == '\0')) {
		warn("%s", filename);
		return -1;
	} else if (rv == 0 && S_ISDIR(file_stat.st_mode)) {
		fprintf(stderr, "%s: is a directory\n", filename);
		return -1;
	}

	if (flags != 0)
		fsu_cook_buf(filename, flags);
	else
		return fsu_raw_cat(filename);

	return rv;
}

/* The code below is adapted from src/bin/cat.c */

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kevin Fall.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

static void
fsu_cook_buf(const char *filename, int flags)
{
	FSU_FILE *file;
	int ch, gobble, line, prev, bflag, eflag, nflag, sflag, tflag, vflag;
	bool from_stdin;

	if (filename[0] == '-' && filename[1] == '\0') {
		from_stdin = true;
		file = NULL;
	} else {
		file = fsu_fopen(filename, "r");
		if (file == NULL) {
			warn("%s", filename);
			return;
		}
		from_stdin = false;
	}

	bflag = flags & FSU_CAT_NOT_NUMBER_BLANK;
	eflag = flags & FSU_CAT_DOLLAR_EOL;
	nflag = flags & FSU_CAT_NUMBER;
	sflag = flags & FSU_CAT_SINGLE_NL;
	tflag = flags & FSU_CAT_TAB;
	vflag = flags & FSU_CAT_NON_PRINTING;

	line = gobble = 0;
	for (prev = '\n';
	     (ch = from_stdin ? getc(stdin) : fsu_fgetc(file)) != EOF;
	     prev = ch) {
		if (prev == '\n') {
			if (ch == '\n') {
				if (sflag) {
					if (!gobble && nflag && !bflag)
						fprintf(stdout, "%6d\t\n",
							++line);
					else if (!gobble && putchar(ch) == EOF)
						break;
					gobble = 1;
					continue;
				}
				if (nflag) {
					if (!bflag) {
						fprintf(stdout, "%6d\t",
							++line);
						if (ferror(stdout))
							break;
					} else if (eflag) {
						fprintf(stdout, "%6s\t", "");
						if (ferror(stdout))
							break;
					}
				}
			} else if (nflag) {
				fprintf(stdout, "%6d\t", ++line);
				if (ferror(stdout))
					break;
			}
		}
		gobble = 0;
		if (ch == '\n') {
			if (eflag)
				if (putchar('$') == EOF)
					break;
		} else if (ch == '\t') {
			if (tflag) {
				if (putchar('^') == EOF || putchar('I') == EOF)
					break;
				continue;
			}
		} else if (vflag) {
			if (!isascii(ch)) {
				if (putchar('M') == EOF || putchar('-') == EOF)
					break;
				ch = toascii(ch);
			}
			if (iscntrl(ch)) {
				if (putchar('^') == EOF ||
				    putchar(ch == '\177' ? '?' :
				    ch | 0100) == EOF)
					break;
				continue;
			}
		}
		if (putchar(ch) == EOF)
			break;
	}

	if (ferror(stdout))
		warn("stdout");
	fsu_fclose(file);
}

/* Adapted from src/bin/cat.c */
static int
fsu_raw_cat(const char *filename)
{
	uint8_t *buf, fb_buf[BUFSIZE];
	size_t bsize;
	ssize_t nr, nw, off;
	off_t roff;
	int fd, rv, wfd;
	struct stat sbuf;
	bool from_stdin;

	from_stdin = filename[0] == '-' && filename[1] == '\0';
	if (!from_stdin)
		fd = rump_sys_open(filename, RUMP_O_RDONLY);
	else
		fd = STDIN_FILENO;

	wfd = fileno(stdout);
	bsize = 0;
	buf = NULL;
	if (fstat(wfd, &sbuf) == 0 && sbuf.st_blksize > sizeof(fb_buf)) {
		bsize = sbuf.st_blksize;
		buf = malloc(bsize);
	}
	if (buf == NULL) {
		bsize = sizeof(fb_buf);
		buf = fb_buf;
	}

	roff = 0;
	for (;;) {
		if (from_stdin)
			nr = read(fd, buf, bsize);
		else
			nr = rump_sys_read(fd, buf, bsize);

		if (nr <= 0)
			break;

		roff += nr;
		for (off = 0; nr; nr -= nw, off += nw)
			if ((nw = write(wfd, buf + off, (size_t)nr)) < 0) {
				warn("stdout");
				rv = -1;
				goto out;
			}
	}

	if (nr < 0) {
		warn("%s", filename);
		rv = -1;
	} else
		rv = 0;

out:
	if (buf != fb_buf)
		free(buf);
	return rv;
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s %s [-benstv] [-] filename\n",
		getprogname(), fsu_mount_usage());

	exit(EXIT_FAILURE);
}
