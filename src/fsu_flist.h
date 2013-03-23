/* $NetBSD: fsu_flist.h,v 1.3 2009/04/12 23:02:43 stacktic Exp $ */

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

#ifndef _FSU_FLIST_H_
#define _FSU_FLIST_H_

struct stat;

#define FSU_FLIST_RECURSIVE (0x01)
#define FSU_FLIST_STATLINK (FSU_FLIST_RECURSIVE<<1)
#define FSU_FLIST_REALFS (FSU_FLIST_STATLINK<<1)

typedef struct fsu_fent_s {
	struct fsu_fent_s *parent;
	unsigned int childno;
	char *path;
	char *filename;
	unsigned int pathlen;
	struct stat sb;
	LIST_ENTRY(fsu_fent_s) next;
} FSU_FENT;

LIST_HEAD(fsu_flist_s, fsu_fent_s);
typedef struct fsu_flist_s fsu_flist;

fsu_flist	*fsu_flist_build(const char *, int);
void		fsu_flist_free(fsu_flist *);
void		fsu_flist_free_entry(FSU_FENT *);

#endif /* !_FSU_FLIST_H_ */
