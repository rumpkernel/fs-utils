/* $NetBSD: mount_msdos.c,v 1.2 2009/11/05 14:02:42 stacktic Exp $ */

/*
 * Copyright (c) 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include "fs-utils.h"

#if defined(__NetBSD__) || defined(__FreeBSD__)
#ifndef lint
__RCSID("$NetBSD: mount_msdos.c,v 1.2 2009/11/05 14:02:42 stacktic Exp $");
#endif /* not lint */
#endif

#include <sys/stat.h>
#include <sys/param.h>

#ifdef __NetBSD__
#include <sys/mount.h>

#include <msdosfs/msdosfsmount.h>
#else
#include "nb_fs.h"
#endif
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef __NetBSD__
#include <util.h>
#endif
#include <mntopts.h>

#include "mountprog.h"
#include "mount_msdos.h"

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_ASYNC,
	MOPT_SYNC,
	MOPT_UPDATE,
	MOPT_GETARGS,
	MOPT_NULL,
};

int
mount_msdos_parseargs(int argc, char **argv,
	struct msdosfs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	struct stat sb;
	int c, set_gid, set_uid, set_mask, set_dirmask, set_gmtoff;
	char *dev, *dir;
	mntoptparse_t mp;

	*mntflags = set_gid = set_uid = set_mask = set_dirmask = set_gmtoff = 0;
	(void)memset(args, '\0', sizeof(*args));

	while ((c = getopt(argc, argv, "Gsl9u:g:m:M:o:t:")) != -1) {
		switch (c) {
		case 'G':
			args->flags |= MSDOSFSMNT_GEMDOSFS;
			break;
		case 's':
			args->flags |= MSDOSFSMNT_SHORTNAME;
			break;
		case 'l':
			args->flags |= MSDOSFSMNT_LONGNAME;
			break;
		case '9':
			args->flags |= MSDOSFSMNT_NOWIN95;
			break;
		case 'u':
			args->uid = a_uid(optarg);
			set_uid = 1;
			break;
		case 'g':
			args->gid = a_gid(optarg);
			set_gid = 1;
			break;
		case 'm':
			args->mask = a_mask(optarg);
			set_mask = 1;
			break;
		case 'M':
			args->dirmask = a_mask(optarg);
			set_dirmask = 1;
			break;
		case 'o':
			mp = getmntopts(optarg, mopts, mntflags, 0);
			if (mp == NULL) {
				warn("getmntopts");
				return 1;
			}
			freemntopts(mp);
			break;
		case 't':
			args->gmtoff = atoi(optarg);
			set_gmtoff = 1;
			break;
		case '?':
		default:
			return 1;
		}
	}

	if (optind + 2 != argc) {
		return 1;
	}

	if (set_mask && !set_dirmask) {
		args->dirmask = args->mask;
		set_dirmask = 1;
	} else if (set_dirmask && !set_mask) {
		args->mask = args->dirmask;
		set_mask = 1;
	}

	dev = argv[optind];
	dir = argv[optind + 1];

	pathadj(dev, canon_dev);
	pathadj(dir, canon_dir);

	args->fspec = dev;
	if (!set_gid || !set_uid || !set_mask) {
		if (stat(dir, &sb) == -1) {
			warn("stat %s", dir);
			return 1;
		}

		if (!set_uid)
			args->uid = sb.st_uid;
		if (!set_gid)
			args->gid = sb.st_gid;
		if (!set_mask) {
			args->mask = args->dirmask =
				sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
		}
	}

	if (!set_gmtoff) {
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
		time_t now;
		struct tm *tm;

		/* use user's time zone as default */
		time(&now);
		tm = localtime(&now);
		args->gmtoff = tm->tm_gmtoff;
#else
		args->gmtoff = 0;
#endif
	}
	args->flags |= MSDOSFSMNT_VERSIONED;
	args->version = MSDOSFSMNT_VERSION;

	return 0;
}
