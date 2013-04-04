/* $NetBSD: fsu_flist.c,v 1.4 2009/11/05 14:39:15 stacktic Exp $ */

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

#include "fs-utils.h"
#include <sys/queue.h>
#include <sys/stat.h>
#ifdef __NetBSD__
#include <sys/syslimits.h>
#else
#define PATH_MAX (1024)
#endif

#include <dirent.h>
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rump/rump_syscalls.h>

#include <fsu_utils.h>

#include "fsu_flist.h"

#define ISDOT(a) ((a)[0] == '.' && \
		  ((a)[1] == '\0' || ((a)[1] == '.' && (a)[2] == '\0')))

static FSU_FENT *fsu_flist_alloc(struct dirent *, FSU_FENT *, int);
static FSU_FENT *fsu_flist_alloc_root(const char *, int);

static int (*statfun)(const char *, struct stat *);

fsu_flist
*fsu_flist_build(const char *rootp, int flags)
{
	FSU_FENT *cur, *prev, *child;
	FSU_DIR *curdir;
	DIR *rcurdir;
	struct dirent *dent;
	fsu_flist *head;

	if ((!(flags & FSU_FLIST_REALFS)) || rootp == NULL)
		return NULL;

	curdir = NULL;
	rcurdir = NULL;

	if (flags & FSU_FLIST_REALFS)
		statfun = NULL;
	else
		statfun = flags & FSU_FLIST_STATLINK ? rump_sys_lstat : rump_sys_stat;

	head = malloc(sizeof(fsu_flist));
	if (head == NULL)
		return NULL;
	LIST_INIT(head);

	cur = fsu_flist_alloc_root(rootp, flags);
	if (cur == NULL)
		return NULL;

	LIST_INSERT_HEAD(head, cur, next);

	LIST_FOREACH(cur, head, next) {
		if (!S_ISDIR(cur->sb.st_mode))
			continue;

		if (flags & FSU_FLIST_REALFS)
			rcurdir = opendir(cur->path);
		else
			curdir = fsu_opendir(cur->path);

		if (curdir == NULL && rcurdir == NULL)
			continue;

		prev = cur;
		for (;;) {
			if (flags & FSU_FLIST_REALFS)
				dent = readdir(rcurdir);
			else
				dent = fsu_readdir(curdir);

			if (dent == NULL)
				break;

			if (ISDOT(dent->d_name) || dent->d_name[0] == '\0')
				continue;

			child = fsu_flist_alloc(dent, cur, flags);
			if (child == NULL)
				continue;

			LIST_INSERT_AFTER(prev, child, next);
			prev = child;

			cur->childno++;
		}
		if (flags & FSU_FLIST_REALFS)
			closedir(rcurdir);
		else
			fsu_closedir(curdir);

		if (!(flags & FSU_FLIST_RECURSIVE))
			break;
	}

	return head;
}

static FSU_FENT
*fsu_flist_alloc(struct dirent *dent, FSU_FENT *parent,
		 int flags)
{
	FSU_FENT *child;
	int rv;
	bool is_child_of_slash;

	is_child_of_slash = (parent->path[0] == '/' && parent->path[1] == '\0');

	child = malloc(sizeof(FSU_FENT));
	if (child == NULL) {
		warn("malloc");
		return NULL;
	}

	child->parent = parent;

	if (is_child_of_slash)
		child->pathlen = parent->pathlen +
#ifdef __linux__
		strlen(dent->d_name);
#else
		dent->d_namlen;
#endif
	else
		child->pathlen = parent->pathlen + 1 +
#ifdef __linux__
		strlen(dent->d_name);
#else
		dent->d_namlen;
#endif

	child->path = malloc(child->pathlen + 1);
	if (child->path == NULL) {
		free(child);
		return NULL;
	}

	if (is_child_of_slash)
		rv = snprintf(child->path, child->pathlen + 1, "/%s",
			      dent->d_name);
	else
		rv = snprintf(child->path, child->pathlen + 1, "%s/%s",
			      parent->path, dent->d_name);

	if (rv != (int)child->pathlen) {
		warn("%s/%s", is_child_of_slash ? "" : parent->path,
		     dent->d_name);
		fsu_flist_free_entry(child);
		return NULL;
	}

	child->filename = child->path + child->pathlen -
#ifdef __linux__
		strlen(dent->d_name);
#else
		dent->d_namlen;
#endif
	child->childno = 0;

	if (flags & FSU_FLIST_REALFS) {
		if (flags & FSU_FLIST_STATLINK)
			rv = lstat(child->path, &child->sb);
		else
			rv = stat(child->path, &child->sb);
	} else
		rv = statfun(child->path, &child->sb);
	if (rv == -1) {
		warn("%s", child->path);
		fsu_flist_free_entry(child);
		return NULL;
	}
	return child;
}

static FSU_FENT
*fsu_flist_alloc_root(const char *path, int flags)
{
	FSU_FENT *root;
	char *p;
	int rv;

	root = malloc(sizeof(FSU_FENT));
	if (root == NULL) {
		warn("malloc");
		return NULL;
	}

	root->parent = NULL;
	root->childno = 0;
	root->path = strdup(path);

	p = strrchr(root->path, '/');
	if (p == NULL || (p == root->path && root->path[1] == '\0'))
		root->filename = root->path;
	else
		root->filename = p + 1;

	root->pathlen = strlen(root->path);

	if (flags & FSU_FLIST_REALFS)
		rv = stat(root->path, &root->sb);
	else
		rv = statfun(root->path, &root->sb);
	if (rv == -1) {
		warn("root stat: %s", root->path);
		fsu_flist_free_entry(root);
		return NULL;
	}
	return root;
}

void
fsu_flist_free(fsu_flist *list)
{
	FSU_FENT *cur;

	while (!LIST_EMPTY(list)) {
		cur = LIST_FIRST(list);
		LIST_REMOVE(cur, next);

		free(cur->path);
		free(cur);
	}
	free(list);
}

void
fsu_flist_free_entry(FSU_FENT *ent)
{

	if (ent == NULL)
		return;

	free(ent->path);
	free(ent);
}
