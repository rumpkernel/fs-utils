/* $NetBSD: mount_ntfs.c,v 1.3 2009/11/06 11:47:41 stacktic Exp $ */

/*
 * Copyright (c) 1994 Christopher G. Demetriou
 * Copyright (c) 1999 Semen Ustimenko
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
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * Id: mount_ntfs.c,v 1.1.1.1 1999/02/03 03:51:19 semenu Exp
 */

#include "fs-utils.h"

#if defined(__NetBSD__) || defined(__FreeBSD__)
#ifndef lint
__RCSID("$NetBSD: mount_ntfs.c,v 1.3 2009/11/06 11:47:41 stacktic Exp $");
#endif
#endif

#include <sys/stat.h>
#include <sys/param.h>

#ifdef __NetBSD__
#include <sys/mount.h>

#include <ntfs/ntfsmount.h>
#else
#include "nb_fs.h"
#endif
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#ifdef __NetBSD__
#include <util.h>
#endif

#include <mntopts.h>

#include "mountprog.h"
#include "mount_ntfs.h"

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	MOPT_NULL,
};

int
mount_ntfs_parseargs(int argc, char **argv,
	struct ntfs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	struct stat sb;
	int c, set_gid, set_uid, set_mask;
	char *dev, *dir;
	mntoptparse_t mp;

	*mntflags = set_gid = set_uid = set_mask = 0;
	(void)memset(args, '\0', sizeof(*args));

	while ((c = getopt(argc, argv, "aiu:g:m:o:")) !=  -1) {
		switch (c) {
		case 'u':
			args->uid = a_uid(optarg);
			set_uid = 1;
			break;
		case 'g':
			args->gid = a_gid(optarg);
			set_gid = 1;
			break;
		case 'm':
			args->mode = a_mask(optarg);
			set_mask = 1;
			break;
		case 'i':
			args->flag |= NTFS_MFLAG_CASEINS;
			break;
		case 'a':
			args->flag |= NTFS_MFLAG_ALLNAMES;
			break;
		case 'o':
			mp = getmntopts(optarg, mopts, mntflags, 0);
			if (mp == NULL) {
				warn("getmntopts");
				return 1;
			}
			freemntopts(mp);
			break;
		case '?':
		default:
			return 1;
		}
	}

	if (optind + 2 != argc) {
		return 1;
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
		if (!set_mask)
			args->mode = sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
	}
	return 0;
}
