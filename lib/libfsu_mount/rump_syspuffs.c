/*	$NetBSD: rump_syspuffs.c,v 1.3 2009/11/05 14:02:43 stacktic Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Research Foundation of Helsinki University of Technology
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

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/syslimits.h>

#include <assert.h>
#include <err.h>
#include <paths.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rump/rump.h>

#include "rump_syspuffs.h"

#ifdef USE_RUMP
#define err(a,b) do { warn(b); return a; } while (0 /*CONSTCOND*/)
#endif

int
mount_syspuffs_parseargs(int argc, char *argv[],
	struct syspuffs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	struct puffs_kargs *kargs = &args->us_kargs;
	int *pflags = &args->us_pflags;
	char comfd[16];
	int sv[2];
	size_t len;
	int rv;

	if (argc < 2) {
#ifdef USE_RUMP
		return 1;
#else
		usage();
#endif
	}

	/* Create sucketpair for communication with the real file server */
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sv) == -1)
		err(1, "socketpair");

#ifndef USE_RUMP
	if ((rv = rump_init()) == -1)
		err(1, "rump_init");
#endif

	switch (fork()) {
	case 0:
		close(sv[1]);
		snprintf(comfd, sizeof(sv[0]), "%d", sv[0]);
		if (setenv("PUFFS_COMFD", comfd, 1) == -1)
			err(1, "setenv");

		argv++;
		if (execvp(argv[0], argv) == -1)
			err(1, "execvp");
		/*NOTREACHED*/
	case -1:
		err(1, "fork");
		/*NOTREACHED*/
	default:
		close(sv[0]);
		break;
	}

	/* read args */
	if (read(sv[1], &len, sizeof(len)) != sizeof(len))
		err(1, "mp 1");
	if (len > MAXPATHLEN)
		err(1, "mntpath > MAXPATHLEN");
	if ((size_t)read(sv[1], canon_dir, len) != len)
		err(1, "mp 2");
	if (read(sv[1], &len, sizeof(len)) != sizeof(len))
		err(1, "fn 1");
	if (len > MAXPATHLEN)
		err(1, "devpath > MAXPATHLEN");
	if ((size_t)read(sv[1], canon_dev, len) != len)
		err(1, "fn 2");
	if (read(sv[1], mntflags, sizeof(*mntflags)) != sizeof(*mntflags))
		err(1, "mntflags");
	if (read(sv[1], kargs, sizeof(*kargs)) != sizeof(*kargs))
		err(1, "puffs_args");
	if (read(sv[1], pflags, sizeof(*pflags)) != sizeof(*pflags))
		err(1, "pflags");

	/* XXX: some adjustments */
	*pflags |= PUFFS_KFLAG_NOCACHE;
	*pflags &= ~PUFFS_FLAG_BUILDPATH;

#if defined(COMPAT_50)
	rv = syspuffs_glueinit(sv[1], &kargs->pa_fd);
#elif defined(HAVE_RUMP_PUB)
	rv = rump_pub_syspuffs_glueinit(sv[1], &kargs->pa_fd);
#else
	rv = rump_syspuffs_glueinit(sv[1], &kargs->pa_fd);
#endif
	assert(rv == 0);

	return 0;
}
