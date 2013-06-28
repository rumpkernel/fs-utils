/*	$NetBSD: mount_v7fs.c,v 1.1 2011/06/27 11:52:58 uch Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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

#include "fs-utils.h"

#if defined(__NetBSD__) || defined(__FreeBSD__)
#ifndef lint
__RCSID("$NetBSD: mount_v7fs.c,v 1.1 2011/06/27 11:52:58 uch Exp $");
#endif /* not lint */
#endif

#include <sys/param.h>
#ifdef __NetBSD__
#include <sys/mount.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include <mntopts.h>

#include "mountprog.h"

#ifdef __NetBSD__
#include <fs/v7fs/v7fs_args.h>
#else
#include <nb_fs.h>
#endif
#include "mount_v7fs.h"

#include <fsu_utils.h>

/*
 * Passed to the rump kernel, so define locally to match NetBSD ones.
 * Maybe should use rumpdefs.h here.
 */
#define RUMP_LITTLE_ENDIAN	1234
#define RUMP_BIG_ENDIAN		4321
#define RUMP_PDP_ENDIAN		3412

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_UPDATE,
	MOPT_GETARGS,
	MOPT_NULL,
};

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{

	return mount_v7fs(argc, argv);
}
#endif

int
mount_v7fs_parseargs(int argc, char **argv, struct v7fs_args *args,
    int *mntflags, char *canon_dev, char *canon_dir)
{
	int ch;
	mntoptparse_t mp;
#ifdef WORDS_BIGENDIAN
	int endian = RUMP_BIG_ENDIAN;
#else
	int endian = RUMP_LITTLE_ENDIAN;
#endif
	*mntflags = 0;
#ifndef HAVE_GETOPT_OPTRESET
	optind = 1;		/* Reset for parse of new argv. */
#else
	optind = optreset = 1;		/* Reset for parse of new argv. */
#endif
	while ((ch = getopt(argc, argv, "o:B:")) != -1)
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, mopts, mntflags, 0);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case 'B':
		  switch (optarg[0]) {
		    case 'l':
		      endian = RUMP_LITTLE_ENDIAN;
		      break;
		    case 'b':
		      endian = RUMP_BIG_ENDIAN;
		      break;
		    case 'p':
		      endian = RUMP_PDP_ENDIAN;
		      break;
		    }
		  break;
		case '?':

		default:
			return 1;
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		return 1;

	pathadj(argv[0], canon_dev);
	args->endian = endian;
	args->fspec = canon_dev;
	pathadj(argv[1], canon_dir);

	return 0;
}
