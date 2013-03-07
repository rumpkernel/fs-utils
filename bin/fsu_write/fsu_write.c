/*	$NetBSD: fsu_write.c,v 1.2 2009/11/05 14:39:15 stacktic Exp $	*/

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

#if HAVE_NBCOMPAT_H
#include <nbcompat.h>
#endif

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fsu_mount.h>
#include <fsu_utils.h>



static void	usage(void);
int		fsu_write(int, const char *, int);

int
main(int argc, char *argv[])
{
	int append, rv;

	setprogname(argv[0]);

#ifdef USE_RUMP
	if (fsu_mount(&argc, &argv) == 0)
		errx(-1, NULL);
#endif

	append = 0;
	while ((rv = getopt(argc, argv, "a")) != -1) {
		switch(rv) {
		case 'a':
			append = 1;
			break;

		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	}

	if (optind >= argc)
		usage();

	rv = fsu_write(STDIN_FILENO, argv[optind], append);

	return rv != 0;
}

int
fsu_write(int fd, const char *fname, int append)
{
	int rd, wr;
	size_t total;
	uint8_t buf[8192];
	FSU_FILE *fdout;

	if (fname == NULL)
		return -1;

	fdout = fsu_fopen(fname, append ? "a" : "w");
	if (fdout == NULL) {
		warn("%s", fname);
		return -1;
	}

	total = 0;
	do {
		rd = read(fd, buf, sizeof(buf));
		if (rd == -1) {
			warn("read");
			goto out;
		}
		wr = fsu_fwrite(buf, 1, rd, fdout);
		if (wr == -1 || wr != rd) {
			warn("%s", fname);
			goto out;
		}
		total += wr;
	} while (rd > 0 && errno != ENOSPC);

out:
	fsu_fclose(fdout);
	return total;
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s %s [-a] file\n",
		getprogname(), fsu_mount_usage());

	exit(EXIT_FAILURE);
}
