/* $NetBSD: mount_udf.c,v 1.2 2009/11/05 14:02:42 stacktic Exp $ */

/*
 * Copyright (c) 2006 Reinoud Zandijk
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
 */

#include "fs-utils.h"

#if defined(__NetBSD__) || defined(__FreeBSD__)
#ifndef lint
__RCSID("$NetBSD: mount_udf.c,v 1.2 2009/11/05 14:02:42 stacktic Exp $");
#endif /* not lint */
#endif

#include <sys/stat.h>
#include <sys/param.h>
#ifdef __NetBSD__
#include <sys/mount.h>
#endif

#include <assert.h>
#include <errno.h>
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

/* mount specific options */
#ifdef __NetBSD__
#include <fs/udf/udf_mount.h>
#else
#include "nb_fs.h"
#endif
#include <mntopts.h>

#include "mountprog.h"
#include "mount_udf.h"

/* options to pass on to the `mount' call */
static const struct mntopt mopts[] = {
	MOPT_STDOPTS,		/* `normal' options		*/
	MOPT_ASYNC,		/* default			*/
	MOPT_NOATIME,		/* dont update access times	*/
	MOPT_UPDATE,		/* not yet supported		*/
	MOPT_GETARGS,		/* printing			*/
	MOPT_NULL,
};

/* main routine */
int
mount_udf_parseargs(int argc, char **argv,
	struct udf_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	uid_t	 anon_uid, nobody_uid;
	gid_t	 anon_gid, nobody_gid;
	int	 ch, set_gmtoff;
	uint32_t sector_size;
	mntoptparse_t mp;

	/* initialise */
	(void)memset(args, 0, sizeof(*args));

	set_gmtoff = *mntflags = 0;
	sector_size = 0;

	/* get nobody */
	nobody_uid = anon_uid = a_uid("nobody");
	nobody_gid = anon_gid = a_gid("nobody");

	while ((ch = getopt(argc, argv, "cg:o:s:t:u:")) != -1) {
		switch (ch) {
#ifdef notyet
		case 'c' :
			args->udfmflags |= UDFMNT_CLOSESESSION;
			break;
#endif
		case 'g' :
			/* convert groupname or numeric equiv. */
			anon_gid = a_gid(optarg);
			break;
		case 'u' :
			/* convert username or numeric equiv. */
			anon_uid = a_uid(optarg);
			break;
		case 'o' :
			/* process generic mount options */
			mp = getmntopts(optarg, mopts, mntflags, 0);
			if (mp == NULL) {
				warn("getmntopts");
				return 1;
			}
			freemntopts(mp);
			break;
		case 's' :
			args->sessionnr = a_num(optarg, "session number");
			break;
		case 't' :
			args->gmtoff = a_num(optarg, "gmtoff");
			set_gmtoff  = 1;
			break;
		default  :
			return 1;
			/* NOTREACHED */
		}
	}

	if (optind + 2 != argc)
		return 1;

	if (!set_gmtoff) {
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
		time_t	 now;
		struct tm *tm;

		/* use user's time zone as default */
		(void)time(&now);
		tm = localtime(&now);
		args->gmtoff = tm->tm_gmtoff;
#else
		args->gmtoff = 0;
#endif
	}

	/* get device and directory specifier */
	pathadj(argv[optind], canon_dev);
	pathadj(argv[optind+1], canon_dir);

	args->version = UDFMNT_VERSION;
	args->fspec = canon_dev;
	args->anon_uid    = anon_uid;
	args->anon_gid    = anon_gid;
	args->nobody_uid  = nobody_uid;
	args->nobody_gid  = nobody_gid;
	args->sector_size = sector_size;		/* invalid */

	return 0;
}
