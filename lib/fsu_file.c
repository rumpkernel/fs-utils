/* $NetBSD: fsu_file.c,v 1.3 2009/11/05 14:02:43 stacktic Exp $ */

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

#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rump/rump_syscalls.h>

#include <fsu_utils.h>

static void	fsu_fill_buffer(FSU_FILE *);

char
fsu_fgetc(FSU_FILE *file)
{

	assert(file != NULL);

	if ((file->fd_mode & FSU_FILE_READ) == 0) {
		errno = EBADF;
		return EOF;
	}

	if (file->fd_fpos == file->fd_last) {
		if (file->fd_eof || file->fd_err != 0)
			return EOF;

		fsu_fill_buffer(file);
	}
	++file->fd_fpos;

	return file->fd_buf[file->fd_bpos++];
}

int
fsu_fputc(int c, FSU_FILE *file)
{

	assert(file != NULL);

	if ((file->fd_mode & FSU_FILE_WRITE) == 0) {
		errno = EBADF;
		return EOF;
	}

	if (file->fd_fpos == file->fd_last) {
		if (file->fd_eof || file->fd_err != 0)
			return EOF;

		fsu_fill_buffer(file);
	}
	++file->fd_fpos;
	file->fd_dirty = true;

	return (file->fd_buf[file->fd_bpos++] = c);
}


FSU_FILE
*fsu_fopen(const char *fname, const char *mode)
{
	FSU_FILE *file;
	struct stat sb;
	int rv, flags;
	bool exists;
	mode_t mask;

	umask((mask = umask(0)));
	mask = ~mask;
	flags = 0;

	exists = (rump_sys_stat(fname, &sb) == 0);
#ifdef EFTYPE
	if (strchr(mode, 'f') != NULL && !S_ISREG(sb.st_mode)) {
		errno = EFTYPE;
		return NULL;
	}
#endif
	file = malloc(sizeof(FSU_FILE));
	if (file == NULL)
		return NULL;

	file->fd_err = 0;
	file->fd_fpos = file->fd_bpos = file->fd_last = 0;
	file->fd_mode = 0;
	file->fd_eof = file->fd_dirty = false;

	switch(mode[0]) {
		case 'r':
			flags |= O_RDONLY;
			break;
		case 'w':
			flags |= O_WRONLY | O_CREAT | O_TRUNC;
			break;
		case 'a':
			flags |= O_WRONLY | O_CREAT | O_APPEND;
			file->fd_fpos = file->fd_last = sb.st_size;
			break;

	}

	if (strchr(mode, '+') != NULL)
		flags |= O_RDWR;

	rv = rump_sys_open(fname, flags, 0666 & mask);
	if (rv == -1) {
		free(file);
		return NULL;
	}
	file->fd_fd = rv;
	file->fd_mode = flags & O_WRONLY ? FSU_FILE_WRITE : FSU_FILE_READ;
	return file;
}

void
fsu_fclose(FSU_FILE *file)
{

	assert(file != NULL);

	fsu_fflush(file);
	rump_sys_close(file->fd_fd);
	free(file);
}

void
fsu_rewind(FSU_FILE *file)
{

	assert(file != NULL);

	fsu_fflush(file);

	file->fd_fpos = file->fd_bpos = file->fd_last = 0;

	fsu_fill_buffer(file);
}

bool
fsu_feof(FSU_FILE *file)
{

	assert(file != NULL);

	return file->fd_eof;
}

void
fsu_clearerr(FSU_FILE *file)
{

	assert(file != NULL);

	file->fd_err = 0;
}

int
fsu_ferror(FSU_FILE *file)
{

	assert(file != NULL);

	return file->fd_err;
}


size_t
fsu_fwrite(void *ptr, size_t size, size_t nmemb, FSU_FILE *file)
{
	int rv;
	ssize_t rsize;
	unsigned char *p;

	assert(file != NULL);

	if ((rsize = size * nmemb) == 0)
		return 0;

	if ((file->fd_mode & FSU_FILE_WRITE) == 0) {
		errno = EBADF;
		return 0;
	}

	if (file->fd_err != 0)
		return 0;

	p = ptr;
	if (rsize > (ssize_t)(sizeof(file->fd_buf) - file->fd_bpos)) {

		fsu_fflush(file);

		while (rsize > (ssize_t)sizeof(file->fd_buf)) {
			rv = rump_sys_pwrite(file->fd_fd, p, sizeof(file->fd_buf), file->fd_fpos);
			if (rv == -1) {
				file->fd_err = errno;
				return 0;
			}
			p += rv;
			rsize -= rv;
			file->fd_fpos += rv;
		}
		rv = rump_sys_pwrite(file->fd_fd, p, rsize, file->fd_fpos);
		if (rv == -1) {
			file->fd_err = errno;
			return 0;
		}

		file->fd_fpos += rv;

		if (rv == rsize) {
			file->fd_last = file->fd_fpos;
			fsu_fill_buffer(file);
		} else
			nmemb = (rsize - rv) / size;
	} else {
		memcpy(file->fd_buf + file->fd_bpos, ptr, rsize);
		file->fd_fpos += rsize;
		file->fd_bpos += rsize;
		file->fd_dirty = true;
	}

	return nmemb;
}

int
fsu_fflush(FSU_FILE *file)
{
	int rv;

	assert(file != NULL);

	if ((file->fd_mode & FSU_FILE_WRITE) == 0) {
		errno = EBADF;
		return EOF;
	}

	if (file->fd_dirty) {
		rv = rump_sys_pwrite(file->fd_fd, file->fd_buf,
				file->fd_bpos,
				file->fd_fpos - file->fd_bpos);
		if (rv == -1 || rv != (int)file->fd_bpos)
			file->fd_err = errno;
		file->fd_dirty = false;
	}
	return 0;
}

int
fsu_fseek(FSU_FILE *file, long off, int whence)
{

	return fsu_fseeko(file, (off_t)off, whence);
}

int
fsu_fseeko(FSU_FILE *file, off_t off, int whence)
{
	struct stat sb;

	fsu_fflush(file);
	fsu_clearerr(file);
	rump_sys_fstat(file->fd_fd, &sb);

	switch (whence) {
	case SEEK_CUR:
		file->fd_fpos += off;
		break;
	case SEEK_SET:
		file->fd_fpos = off;
		break;
	case SEEK_END:
		file->fd_fpos = sb.st_size + off;
		break;
	default:
		errno = EINVAL;
		return -1;
		/* NOTREACHED */
	}
	if (file->fd_fpos >= (size_t)sb.st_size) {
		file->fd_last = file->fd_fpos;
		file->fd_bpos = 0;
	} else
		fsu_fill_buffer(file);
	return 0;
}

long int
fsu_ftell(FSU_FILE *file)
{

	return (long int)file->fd_fpos;
}

off_t
fsu_ftello(FSU_FILE *file)
{

	return (off_t)file->fd_fpos;
}

static void
fsu_fill_buffer(FSU_FILE *file)
{
	int rv;

	assert(file != NULL);

	fsu_fflush(file);

	rv = rump_sys_read(file->fd_fd, file->fd_buf, sizeof(file->fd_buf));

	if (rv == -1) {
		file->fd_err = errno;
  		file->fd_eof = true;
		return;
	} else if (rv < (int)sizeof(file->fd_buf))
		file->fd_eof = true;

	file->fd_last = file->fd_fpos + rv;
	file->fd_bpos = 0;
}

/* from src/lib/libc/stdio/fread.c */
/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
size_t
fsu_fread(void *ptr, size_t size, size_t count, FSU_FILE *file)
{
	size_t resid;
	char *p;
	int r;
	size_t total;

	assert(file != NULL);

	if ((resid = count * size) == 0)
		return 0;

	assert(ptr != NULL);

	total = resid;
	p = ptr;

	fsu_fflush(file);

	while (resid > (size_t)(r = file->fd_last - file->fd_fpos)) {
		memcpy(p, file->fd_buf + file->fd_bpos, r);

		file->fd_fpos += r;
		p += r;
		resid -= r;

		if (file->fd_eof)
			return ((total - resid) / size);

		fsu_fill_buffer(file);
	}
	memcpy(p, file->fd_buf + file->fd_bpos, resid);

	file->fd_fpos += resid;
	file->fd_bpos += resid;

	return (count);
}
