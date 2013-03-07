/* $NetBSD: fattr.c,v 1.3 2009/11/05 14:02:42 stacktic Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: fattr.c,v 1.3 2009/11/05 14:02:42 stacktic Exp $");
#endif /* not lint */
#endif

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mountprog.h"

int
a_num(const char *s, const char *id_type)
{
	int id;
	char *ep;

	id = strtol(s, &ep, 0);
	if (*ep || s == ep || id < 0) {
#ifdef USE_RUMP
		warn("unknown %s id: %s", id_type, s);
		return -1;
#else
		errx(1, "unknown %s id: %s", id_type, s);
#endif
	}
	return id;
}

gid_t
a_gid(const char *s)
{
	struct group *gr;

	if ((gr = getgrnam(s)) != NULL)
		return gr->gr_gid;
	return a_num(s, "group");
}

uid_t
a_uid(const char *s)
{
	struct passwd *pw;

	if ((pw = getpwnam(s)) != NULL)
		return pw->pw_uid;
	return a_num(s, "user");
}

mode_t
a_mask(const char *s)
{
	int rv;
	char *ep;

	rv = strtol(s, &ep, 8);
	if (s == ep || *ep || rv < 0) {
#ifdef USE_RUMP
		warn("invalid file mode: %s", s);
		return -1;
#else
		errx(1, "invalid file mode: %s", s);
#endif
	}
	return rv;
}
