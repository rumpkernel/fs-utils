/* $NetBSD: fsu_dir.c,v 1.6 2009/11/18 10:02:26 stacktic Exp $ */

/*
 * Copyright (c) 2008,2009 Arnaud Ysmal.  All Rights Reserved.
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

#define _DIRENT_SIZE(dp) ((dp)->d_reclen)
#define _DIRENT_NEXT(dp) ((void *)(((char *)(dp)) + (dp)->d_reclen))
#endif

#include <sys/stat.h>

#if HAVE_NBCOMPAT_H
#include <nbcompat.h>
#endif

#include <assert.h>
#include <dirent.h>
#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rump/rump_syscalls.h>
#include <rump/rumpdefs.h>
#include <fsu_utils.h>

static char *fsu_getdirname(void);

FSU_DIR
*fsu_opendir(const char *path)
{
	FSU_DIR *dir;

	assert(path != NULL);

	dir = malloc(sizeof(FSU_DIR));
	if (dir == NULL)
		return NULL;
        memset(dir, 0, sizeof(FSU_DIR));

	dir->dd_path = strdup(path);
	dir->dd_fd = rump_sys_open(dir->dd_path, RUMP_O_RDONLY|RUMP_O_DIRECTORY);

	if (dir->dd_fd  == -1) {
		free(dir->dd_path);
		free(dir);
		return NULL;
	}
	return dir;
}

void
fsu_closedir(FSU_DIR *dir)
{

	assert(dir != NULL);

	free(dir->dd_path);
	free(dir);
}

struct dirent
*fsu_readdir(FSU_DIR *dir)
{
	struct dirent *dent;

	assert(dir != NULL);

 retry:
	if (dir->dd_size <= 0) {
		dir->dd_size = rump_sys_getdents(dir->dd_fd, (char*)dir->dd_buf,
			sizeof(dir->dd_buf));
		if (dir->dd_size <= 0)
			return NULL;

		dir->dd_dent = (struct dirent *)dir->dd_buf;
	}

	dent = dir->dd_dent;
	dir->dd_size -= _DIRENT_SIZE(dir->dd_dent);
	dir->dd_dent = _DIRENT_NEXT(dir->dd_dent);

	/* don't return dirents for removed files */
	if (dent->d_ino == 0)
		goto retry;

	return dent;
}

void
fsu_rewinddir(FSU_DIR *dir)
{

	assert(dir != NULL);

	dir->dd_off = 0;
	dir->dd_size = 0;
}

char *
fsu_getapath(const char *dir)
{
	size_t new_size, old_size;
	char *new_dir, *old_dir, *p;
	const char *fname;

	fname = p = NULL;
	old_dir = fsu_getcwd();

	if (rump_sys_chdir(dir) != 0) {
		new_dir = strdup(dir);
		p = strrchr(new_dir, '/');
		if (p != NULL) {
			*p = 0;
			fname = dir + (p - new_dir) + 1;
			if (rump_sys_chdir(new_dir) != 0) {
				free(new_dir);
				free(old_dir);
				return NULL;
			}
		} else
			fname = dir;
		free(new_dir);
	}
	new_dir = fsu_getcwd();
	new_size = old_size = strlen(new_dir);

	if (fname != NULL) {
		if (new_size == 1) {
			new_size = strlen(fname) + 2;
			old_size = 0;
		} else
			new_size += strlen(fname) + 2; /* '/' and '\0' */

		new_dir = realloc(new_dir, new_size);
		new_dir[old_size] = '/';
		new_dir[old_size + 1] = 0;
		strlcat(new_dir, fname, new_size);
		new_dir[new_size - 1] = 0;
	}

	rump_sys_chdir(old_dir);
	free(old_dir);
	return new_dir;
}

#define ALLOC_SIZE (10)

/* Gets the absolute path of the current directory */
char
*fsu_getcwd(void)
{
	int i, rv;
	char **names, **tmpnames;
	char *dirname;
	size_t len;

	names = malloc(ALLOC_SIZE * sizeof(char *));
	if (names == NULL)
		return NULL;

	len = 1;

	for (i = 0;; ++i) {
		names[i] = fsu_getdirname();
		if (names[i] == NULL)
			goto err;

                if (names[i][0] == '/' && names[i][1] == '\0') {
                        len += 1;
			break;
		}

		len += strlen(names[i]) + 1;

		rv = rump_sys_chdir("..");
		if (rv == -1)
			goto err;

		if ((i + 1) % ALLOC_SIZE == 0) {
			tmpnames = realloc(names,
			    (i + 1 + ALLOC_SIZE) * sizeof(char *));
			if (tmpnames == NULL)
				goto err;
			names = tmpnames;
		}
	}

	if (i == 0) {
                dirname = names[0];
                free(names);
		return dirname;
        }
	dirname = malloc(len);
	if (dirname == NULL)
		goto err;

        dirname[0] = '/';
	dirname[1] = '\0';
	free(names[i]);
	for (--i; i >= 0; --i) {
		rv = strlcat(dirname, names[i], len);
		free(names[i]);
		dirname[rv] = '/';
		dirname[rv + 1] = '\0';
	}
	free(names);

	rump_sys_chdir(dirname);
	return dirname;

err:
	for (--i; i >= 0; --i) {
		rump_sys_chdir(names[i]);
		free(names[i]);
	}
	free(names);
	return NULL;
}

static ino_t root_ino = 0;

/* Gets the name of the current directory */
static char
*fsu_getdirname(void)
{
	struct stat dot;
	struct dirent *dent;
	int fd, rv, size;
	uint8_t buf[8192];
	off_t off;

	off = 0;

	if (root_ino == 0) {
		rv = rump_sys_stat("/", &dot);
		if (rv == -1)
			return NULL;
		root_ino = dot.st_ino;
	}

	rv = rump_sys_stat(".", &dot);
	if (rv == -1)
		return NULL;

	if (dot.st_ino == root_ino)
		return strdup("/");

	fd = rump_sys_open("..", RUMP_O_RDONLY|RUMP_O_DIRECTORY);
	assert(fd >= 0);
	for (;;) {
		size = rump_sys_getdents(fd, (char *)buf, sizeof(buf));
		if (size == -1) {
			warn("..");
			return NULL;
		} else if (size == 0)
			return NULL;
		dent = (struct dirent *)buf;

		while (size > 0) {
			if (dent->d_fileno == dot.st_ino)
				return strdup(dent->d_name);
			size -= _DIRENT_SIZE(dent);
			dent = _DIRENT_NEXT(dent);
		}
	}
	/* NOTREACHED */
}
