/* $NetBSD: fsu_utils.h,v 1.2 2009/11/05 14:02:43 stacktic Exp $ */

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

#ifndef _FSU_UTILS_H_
#define _FSU_UTILS_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __linux__
#include <sys/types.h>

#include <bsd/stdlib.h>
#include <bsd/string.h>
#include <bsd/unistd.h>

#define user_from_uid(a, b) (NULL)
#define group_from_gid(a, b) (NULL)
#define uid_from_user(a, b) (-1)

#endif


/* File Descriptor */

typedef struct {
	int fd_fd;
        char *fd_name;          /* filename */
        uint8_t fd_buf[8192];   /* current buffer */
        size_t fd_bpos;         /* position in the buffer */
        size_t fd_fpos;         /* position in the file */
        size_t fd_last;         /* last position in the file buffered */
        bool fd_eof;            /* has the buffer reached the end of file */
        int fd_err;             /* saving last errno */

#define FSU_FILE_READ           (1)
#define FSU_FILE_WRITE          (2)
#define FSU_FILE_READWRITE      (3)
        uint8_t fd_mode;        /* access mode */

        bool fd_dirty;          /* has the buffer been modified */
} FSU_FILE;

/* Directory descriptor */
typedef struct {
	int dd_fd;
        char *dd_path;          /* dirname */
        uint8_t dd_buf[8192];   /* current buffer */
        off_t dd_off;           /* position in the directory */
        int dd_size;            /* size returned by last getdents */
        struct dirent *dd_dent; /* current dir entry */
} FSU_DIR;

/* Files */
FSU_FILE        *fsu_fopen(const char *, const char *);
char            fsu_fgetc(FSU_FILE *);
int             fsu_fputc(int, FSU_FILE *);
void            fsu_fclose(FSU_FILE *);
void            fsu_rewind(FSU_FILE *);
bool            fsu_feof(FSU_FILE *);
void            fsu_clearerr(FSU_FILE *);
int             fsu_ferror(FSU_FILE *);
size_t          fsu_fread(void *, size_t, size_t, FSU_FILE *);
size_t          fsu_fwrite(void *, size_t, size_t, FSU_FILE *);
int		fsu_fflush(FSU_FILE *);
int		fsu_fseek(FSU_FILE *, long, int);
int		fsu_fseeko(FSU_FILE *, off_t, int);
long int	fsu_ftell(FSU_FILE *);
off_t		fsu_ftello(FSU_FILE *);

/* Directory */
FSU_DIR         *fsu_opendir(const char *);
struct dirent   *fsu_readdir(FSU_DIR *);
void            fsu_closedir(FSU_DIR *);
void            fsu_rewinddir(FSU_DIR *);
char            *fsu_getcwd(void);
char		*fsu_getapath(const char *);

/* Others */
char *fsu_str2arg(char *, int *, char **, int);
int fsu_str2argc(char *);
#endif /* !_FSU_UTILS_H_ */
