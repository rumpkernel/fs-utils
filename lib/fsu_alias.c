/* $NetBSD: fsu_alias.c,v 1.1 2009/11/05 14:02:42 stacktic Exp $ */

/*
 * Copyright (c) 2009 Arnaud Ysmal.  All Rights Reserved.
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

#include "fs-utils.h"

#if defined(__NetBSD__)
#include <sys/syslimits.h>
#elif defined(__FreeBSD__)
#include <sys/param.h>
#else
#define PATH_MAX (1024)
#endif

#include <sys/stat.h>

#if HAVE_NBCOMPAT_H
#include <nbcompat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fsu_utils.h>

#include "fsu_alias.h"

static struct fsu_fsalias_s *alias_head, *alias_tail;

struct fsu_fsalias_s
*get_alias(const char *al)
{
	struct fsu_fsalias_s *cur;

	for (cur = alias_head; cur != NULL; cur = cur->fsa_next)
		if (cur->fsa_name != NULL && strcmp(cur->fsa_name, al) == 0)
			return cur;
	return NULL;
}

int
build_alias_list(void)
{
	struct stat sb;
	struct fsu_fsalias_s *new;
	FILE *fd;
	int i, rv;
	size_t len, off;
	char buf[8192], file[PATH_MAX + 1], *fp, *home;

	fp = getenv("HOME");
	if (fp == NULL)
		return -1;

	len = strlcpy(file, fp, PATH_MAX + 1);
	strlcat(file, "/.fsurc", PATH_MAX + 1);

	/* no file no alias loaded */
	if (stat(file, &sb) != 0 || (fd = fopen(file, "r")) == NULL)
		return -1;

	rv = 0;
	new = NULL;
	for (;;) {
		/* get a line */
		for (i = 0; i < (int)sizeof(buf); ++i) {
			buf[i] = fgetc(fd);
			if (buf[i] == '\n' || feof(fd)) {
				buf[i] = '\0';
				break;
			}
		}

		if (buf[0] == '#')
			continue;
		if (i == 0 && feof(fd))
			break;

		if (new == NULL) {
			new = malloc(sizeof(struct fsu_fsalias_s));
			if (new == NULL) {
				rv = 1;
				goto out;
			}
		}

		fp = strtok(buf, ":");
		if (fp == NULL)
			continue;
		if (fp[0] == '-') {
			fprintf(stderr,
			  "alias starting with a dash ('-') are not suported");
			continue;
		}
		new->fsa_name = strdup(fp);

		fp = strtok(NULL, ":");
		if (fp == NULL)
			continue;

		if (fp[0] == '~' && fp[1] == '/') {
			home = getenv("HOME");

			if (home == NULL)
				continue;

			len = strlen(home) + strlen(fp);
			new->fsa_path = malloc(len);
			if (new->fsa_path == NULL)
				continue;
			off = strlcpy(new->fsa_path, home, len);
			new->fsa_path[off] = '/';
			new->fsa_path[off + 1] = '\0';
			strlcat(new->fsa_path, fp + 2, len);
		} else
			new->fsa_path = strdup(fp);

		fp = strtok(NULL, ":");
		if (fp == NULL)
			continue;
		new->fsa_type = strdup(fp);

		fp = strtok(NULL, ":");
		if (fp == NULL)
			new->fsa_mntopt = NULL;
		else
			new->fsa_mntopt = strdup(fp);

		fp = strtok(NULL, ":");
		if (fp == NULL)
			new->fsa_puffsexec = NULL;
		else
			new->fsa_puffsexec = strdup(fp);

		/* strtok skips zero length tokens */
		if (strcmp(new->fsa_type, "puffs") == 0 &&
		    new->fsa_puffsexec == NULL &&
		    new->fsa_mntopt != NULL) {
			new->fsa_puffsexec = new->fsa_mntopt;
			new->fsa_mntopt = NULL;
		}

		new->fsa_next = NULL;

		if (alias_head == NULL)
			alias_head = alias_tail = new;
		else
			alias_tail = alias_tail->fsa_next = new;
		new = NULL;
	}

out:
	fclose(fd);
	if (new != NULL)
		free(new);
	return rv;
}

void
free_alias_list(void)
{
	struct fsu_fsalias_s *cur;

	while ((cur = alias_head) != NULL) {
		alias_head = alias_head->fsa_next;
		free(cur->fsa_name);
		free(cur->fsa_path);
		free(cur->fsa_type);
		if (cur->fsa_mntopt != NULL)
			free(cur->fsa_mntopt);
		if (cur->fsa_puffsexec != NULL)
			free(cur->fsa_puffsexec);
		free(cur);
	}
}
