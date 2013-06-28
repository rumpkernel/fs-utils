/*	$NetBSD: mount_lfs.c,v 1.2 2009/11/05 14:02:42 stacktic Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_lfs.c	8.4 (Berkeley) 4/26/95";
#else
__RCSID("$NetBSD: mount_lfs.c,v 1.2 2009/11/05 14:02:42 stacktic Exp $");
#endif
#endif /* not lint */
#endif

#include <sys/param.h>

#ifdef __NetBSD__
#include <sys/mount.h>

#include <ufs/ufs/ufsmount.h>
#else
#include "nb_fs.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>

#include <mntopts.h>
#include "pathnames.h"
#include "mountprog.h"
#include "mount_lfs.h"

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_UPDATE,
	MOPT_GETARGS,
	MOPT_NOATIME,
	MOPT_NULL,
};

#ifdef WANT_CLEANER
#include <paths.h>
static void	invoke_cleaner(char *);
static void	kill_daemon(char *);
static void	kill_cleaner(char *);
#endif /* WANT_CLEANER */

static int short_rds, cleaner_debug, cleaner_bytes, fs_idle, noclean;
static const char *nsegs;

int
mount_lfs_parseargs(int argc, char *argv[],
	struct ufs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	int ch;
	mntoptparse_t mp;

	memset(args, 0, sizeof(*args));
	nsegs = "4";
	*mntflags = noclean = 0;
	cleaner_bytes = 1;
	while ((ch = getopt(argc, argv, "bdiN:no:s")) != -1)
		switch (ch) {
		case 'b':
			cleaner_bytes = !cleaner_bytes;
			break;
		case 'd':
			cleaner_debug = 1;
			break;
		case 'i':
			fs_idle = 1;
			break;
		case 'n':
			noclean = 1;
			break;
		case 'N':
			nsegs = optarg;
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
			short_rds = 1;
			break;
		case '?':
		default:
			return 1;
		}
	argc -= optind;
	argv += optind;

	if (argc != 2) {
		return 1;
	}

	pathadj(argv[0], canon_dev);
	args->fspec = canon_dev;

	pathadj(argv[1], canon_dir);

	return 0;
}

#ifdef WANT_CLEANER
static void
kill_daemon(char *pidname)
{
	FILE *fp;
	char s[80];
	pid_t pid;

	fp = fopen(pidname, "r");
	if (fp) {
		fgets(s, 80, fp);
		pid = atoi(s);
		if (pid)
			kill(pid, SIGINT);
		fclose(fp);
	}
}

static void
kill_cleaner(char *name)
{
	char *pidname;
	char *cp;
	int off;

	/* Parent first */
	asprintf(&pidname, "%slfs_cleanerd:m:%s.pid", _PATH_VARRUN, name);
	if (!pidname)
		err(1, "malloc");
	off = strlen(_PATH_VARRUN);
	while((cp = strchr(pidname + off, '/')) != NULL)
		*cp = '|';
	kill_daemon(pidname);
	free(pidname);

	/* Then child */
	asprintf(&pidname, "%slfs_cleanerd:s:%s.pid", _PATH_VARRUN, name);
	if (!pidname)
		err(1, "malloc");
	off = strlen(_PATH_VARRUN);
	while((cp = strchr(pidname + off, '/')) != NULL)
		*cp = '|';
	kill_daemon(pidname);
	free(pidname);
}

static void
invoke_cleaner(char *name)
{
	const char *args[7], **ap = args;

	/* Build the argument list. */
	*ap++ = _PATH_LFS_CLEANERD;
	if (cleaner_bytes)
		*ap++ = "-b";
	if (nsegs) {
		*ap++ = "-n";
		*ap++ = nsegs;
	}
	if (short_rds)
		*ap++ = "-s";
	if (cleaner_debug)
		*ap++ = "-d";
	if (fs_idle)
		*ap++ = "-f";
	*ap++ = name;
	*ap = NULL;

	execv(args[0], __UNCONST(args));
	err(1, "exec %s", _PATH_LFS_CLEANERD);
}
#endif /* WANT_CLEANER */
