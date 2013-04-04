/*	$NetBSD: mount_tmpfs.c,v 1.2 2009/11/05 14:02:42 stacktic Exp $	*/

/*
 * Copyright (c) 2005, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal, developed as part of Google's Summer of Code
 * 2005 program.
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

#include "fs-utils.h"

#ifdef __linux__
#define _XOPEN_SOURCE 500
#define _BSD_SOURCE
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mount_tmpfs.c,v 1.2 2009/11/05 14:02:42 stacktic Exp $");
#endif /* not lint */
#endif

#include <sys/stat.h>
#include <sys/param.h>
#ifdef __NetBSD__
#include <sys/mount.h>

#include <fs/tmpfs/tmpfs_args.h>
#else

#if HAVE_NBCOMPAT_H
#include <nbcompat.h>
#endif

#include "nb_fs.h"
#endif

#ifdef __linux__
#include <bsd/string.h>
#endif

#include <limits.h>
#ifndef LLONG_MAX
#   define LLONG_MAX  9223372036854775807LL
#endif
#ifndef LLONG_MIN
#   define LLONG_MIN  (-LLONG_MAX - 1LL)
#endif

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <grp.h>
#include <mntopts.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mountprog.h"
#include "mount_tmpfs.h"

#ifndef __NetBSD__
int dehumanize_number(const char *, int64_t *);
#endif

/* --------------------------------------------------------------------- */

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	MOPT_NULL,
};

/* --------------------------------------------------------------------- */

int
mount_tmpfs_parseargs(int argc, char *argv[],
	struct tmpfs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	int gidset, modeset, uidset; /* Ought to be 'bool'. */
	int ch;
	gid_t gid;
	uid_t uid;
	mode_t mode;
	int64_t tmpnumber;
	mntoptparse_t mp;
	struct stat sb;

	/* Set default values for mount point arguments. */
	memset(args, 0, sizeof(*args));
	args->ta_version = TMPFS_ARGS_VERSION;
	args->ta_size_max = 0;
	args->ta_nodes_max = 0;
	*mntflags = 0;

	gidset = 0; gid = 0;
	uidset = 0; uid = 0;
	modeset = 0; mode = 0;

#ifdef __linux__
	optind = 1;
#else
	optind = optreset = 1;
#endif
	while ((ch = getopt(argc, argv, "g:m:n:o:s:u:")) != -1 ) {
		switch (ch) {
		case 'g':
			gid = a_gid(optarg);
			gidset = 1;
			break;

		case 'm':
			mode = a_mask(optarg);
			modeset = 1;
			break;

		case 'n':
			if (dehumanize_number(optarg, &tmpnumber) == -1) {
				warn("failed to parse nodes `%s'", optarg);
				return 1;
			}
			args->ta_nodes_max = tmpnumber;
			break;

		case 'o':
			mp = getmntopts(optarg, mopts, mntflags, 0);
			if (mp == NULL) {
				warn("getmntopts");
				return 1;
			}
			freemntopts(mp);
			break;

		case 's':
			if (dehumanize_number(optarg, &tmpnumber) == -1) {
				warn("failed to parse size `%s'", optarg);
				return 1;
			}
			args->ta_size_max = tmpnumber;
			break;

		case 'u':
			uid = a_uid(optarg);
			uidset = 1;
			break;

		case '?':
		default:
			return 1;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		return 1;

	strlcpy(canon_dev, argv[0], MAXPATHLEN);
	pathadj(argv[1], canon_dir);

	if (stat(canon_dir, &sb) == -1) {
		warn("cannot stat `%s'", canon_dir);
		return 1;
	}

	args->ta_root_uid = uidset ? uid : sb.st_uid;
	args->ta_root_gid = gidset ? gid : sb.st_gid;
	args->ta_root_mode = modeset ? mode : sb.st_mode;

	return 0;
}

#ifndef __NetBSD__
int
dehumanize_number(const char *str, int64_t *size)
{
	char *ep, unit;
	const char *delimit;
	long multiplier;
	long long tmp, tmp2;
	size_t len;

	len = strlen(str);
	if (len == 0) {
		errno = EINVAL;
		return -1;
	}

	multiplier = 1;

	unit = str[len - 1];
	if (isalpha((unsigned char)unit)) {
		switch (tolower((unsigned char)unit)) {
		case 'b':
			multiplier = 1;
			break;

		case 'k':
			multiplier = 1024;
			break;

		case 'm':
			multiplier = 1024 * 1024;
			break;

		case 'g':
			multiplier = 1024 * 1024 * 1024;
			break;

		default:
			errno = EINVAL;
			return -1; /* Invalid suffix. */
		}

		delimit = &str[len - 1];
	} else
		delimit = NULL;

	errno = 0;
	tmp = strtoll(str, &ep, 10);
	if (str[0] == '\0' || (ep != delimit && *ep != '\0'))
		return -1; /* Not a number. */
	else if (errno == ERANGE && (tmp == LLONG_MAX || tmp == LLONG_MIN))
		return -1; /* Out of range. */

	tmp2 = tmp * multiplier;
	tmp2 = tmp2 / multiplier;
	if (tmp != tmp2) {
		errno = ERANGE;
		return -1; /* Out of range. */
	}
	*size = tmp * multiplier;

	return 0;
}
#endif
