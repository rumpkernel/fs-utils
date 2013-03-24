/* $NetBSD: fsu_find.c,v 1.2 2009/11/05 14:39:15 stacktic Exp $ */

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

#include <dirent.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fsu_fts.h>
#include <fsu_utils.h>
#include <fsu_mount.h>



static int	fsu_find(const char *, const char *);
static void	usage(void);

int
main(int argc, char *argv[])
{

	setprogname(argv[0]);

	if (fsu_mount(&argc, &argv, MOUNT_READONLY) != 0)
		errx(-1, NULL);

	switch (argc) {
	case 2:
		return fsu_find(".", argv[1]);
	case 3:
		return fsu_find(argv[1], argv[2]);
	default:
		usage();
		return -1;
	}
}

static int
fsu_find(const char *dname, const char *fname)
{
	FSU_FTS *ftsp;
	FSU_FTSENT *cur;
	char *const fts_arg[] = { strdup(dname), NULL };

	ftsp = fsu_fts_open(fts_arg, FTS_LOGICAL, NULL);
	if (ftsp == NULL)
		return -1;

	while ((cur = fsu_fts_read(ftsp)) != NULL) {
		if (cur->fts_info ==  FTS_DP)
			continue;
		if (strcmp(cur->fts_name, fname) == 0)
			printf("%s\n", cur->fts_accpath);
	}
	fsu_fts_close(ftsp);
	free(fts_arg[0]);
	return 0;
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s %s [dirname] filename\n",
		getprogname(), fsu_mount_usage());

	exit(EXIT_FAILURE);
}
