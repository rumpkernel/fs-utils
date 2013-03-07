/*	$NetBSD: ffs.c,v 1.1 2009/04/14 17:37:25 pooka Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Luke Mewburn for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *
 *	@(#)ffs_alloc.c	8.19 (Berkeley) 7/13/95
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: ffs.c,v 1.1 2009/04/14 17:37:25 pooka Exp $");
#endif	/* !__lint */

#include <sys/param.h>

#if !HAVE_NBTOOL_CONFIG_H
#include <sys/mount.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "makefs.h"
#include "ffs.h"

#if HAVE_STRUCT_STATVFS_F_IOSIZE && HAVE_FSTATVFS
#include <sys/statvfs.h>
#endif

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#define FFS_MAXBSIZE (1<<16)

/*
 * Various file system defaults (cribbed from newfs(8)).
 */
#define	DFL_FRAGSIZE		1024		/* fragment size */
#define	DFL_BLKSIZE		8192		/* block size */
#define	DFL_SECSIZE		512		/* sector size */
#define	DFL_CYLSPERGROUP	65536		/* cylinders per group */
#define	DFL_FRAGSPERINODE	4		/* fragments per inode */
#define	DFL_ROTDELAY		0		/* rotational delay */
#define	DFL_NRPOS		1		/* rotational positions */
#define	DFL_RPM			3600		/* rpm of disk */
#define	DFL_NSECTORS		64		/* # of sectors */
#define	DFL_NTRACKS		16		/* # of tracks */

static	void	ffs_dump_fsinfo(fsinfo_t *);
static	void	traverse_tree(fsnode *, fsinfo_t *);


	/* publically visible functions */

void
ffs_prep_opts(fsinfo_t *fsopts)
{
	ffs_opt_t *ffs_opts;

	if ((ffs_opts = calloc(1, sizeof(ffs_opt_t))) == NULL)
		err(1, "Allocating memory for ffs_options");

	fsopts->fs_specific = ffs_opts;

	ffs_opts->bsize= -1;
	ffs_opts->fsize= -1;
	ffs_opts->cpg= -1;
	ffs_opts->density= -1;
	ffs_opts->minfree= -1;
	ffs_opts->optimization= -1;
	ffs_opts->maxcontig= -1;
	ffs_opts->maxbpg= -1;
	ffs_opts->avgfilesize= -1;
	ffs_opts->avgfpdir= -1;
	ffs_opts->version = 1;
}

void
ffs_cleanup_opts(fsinfo_t *fsopts)
{
	if (fsopts->fs_specific)
		free(fsopts->fs_specific);
}

int
ffs_parse_opts(const char *option, fsinfo_t *fsopts)
{
	ffs_opt_t	*ffs_opts = fsopts->fs_specific;

	option_t ffs_options[] = {
		{ "bsize",	&ffs_opts->bsize,	1,	INT_MAX,
					"block size" },
		{ "fsize",	&ffs_opts->fsize,	1,	INT_MAX,
					"fragment size" },
		{ "density",	&ffs_opts->density,	1,	INT_MAX,
					"bytes per inode" },
		{ "minfree",	&ffs_opts->minfree,	0,	99,
					"minfree" },
		{ "maxbpf",	&ffs_opts->maxbpg,	1,	INT_MAX,
					"max blocks per file in a cg" },
		{ "avgfilesize", &ffs_opts->avgfilesize,1,	INT_MAX,
					"expected average file size" },
		{ "avgfpdir",	&ffs_opts->avgfpdir,	1,	INT_MAX,
					"expected # of files per directory" },
		{ "extent",	&ffs_opts->maxbsize,	1,	INT_MAX,
					"maximum # extent size" },
		{ "maxbpcg",	&ffs_opts->maxblkspercg,1,	INT_MAX,
					"max # of blocks per group" },
		{ "version",	&ffs_opts->version,	1,	2,
					"UFS version" },
		{ .name = NULL }
	};

	char	*var, *val;
	int	rv;

	assert(option != NULL);
	assert(fsopts != NULL);
	assert(ffs_opts != NULL);

	if (debug & DEBUG_FS_PARSE_OPTS)
		printf("ffs_parse_opts: got `%s'\n", option);

	if ((var = strdup(option)) == NULL)
		err(1, "Allocating memory for copy of option string");
	rv = 0;

	if ((val = strchr(var, '=')) == NULL) {
		warnx("Option `%s' doesn't contain a value", var);
		goto leave_ffs_parse_opts;
	}
	*val++ = '\0';

	if (strcmp(var, "optimization") == 0) {
		if (strcmp(val, "time") == 0) {
			ffs_opts->optimization = FS_OPTTIME;
		} else if (strcmp(val, "space") == 0) {
			ffs_opts->optimization = FS_OPTSPACE;
		} else {
			warnx("Invalid optimization `%s'", val);
			goto leave_ffs_parse_opts;
		}
		rv = 1;
	} else
		rv = set_option(ffs_options, var, val);

 leave_ffs_parse_opts:
	if (var)
		free(var);
	return (rv);
}


void
ffs_makefs(const char *image, const char *dir, fsnode *root, fsinfo_t *fsopts)
{
	int32_t	ncg = 1;
	ffs_opt_t	*ffs_opts = fsopts->fs_specific;

	assert(image != NULL);
	assert(dir != NULL);
	assert(root != NULL);
	assert(fsopts != NULL);

	if (debug & DEBUG_FS_MAKEFS)
		printf("ffs_makefs: image %s directory %s root %p\n",
		    image, dir, root);

	assert(dir != NULL);
	assert(root != NULL);
	assert(fsopts != NULL);
	assert(ffs_opts != NULL);

	if (debug & DEBUG_FS_VALIDATE) {
		printf("ffs_validate: before defaults set:\n");
		ffs_dump_fsinfo(fsopts);
	}

		/* set FFS defaults */
	if (fsopts->sectorsize == -1)
		fsopts->sectorsize = DFL_SECSIZE;
	if (ffs_opts->fsize == -1)
		ffs_opts->fsize = MAX(DFL_FRAGSIZE, fsopts->sectorsize);
	if (ffs_opts->bsize == -1)
		ffs_opts->bsize = MIN(DFL_BLKSIZE, 8 * ffs_opts->fsize);
	if (ffs_opts->cpg == -1)
		ffs_opts->cpg = DFL_CYLSPERGROUP;
	else
		ffs_opts->cpgflg = 1;
				/* fsopts->density is set below */
	if (ffs_opts->nsectors == -1)
		ffs_opts->nsectors = DFL_NSECTORS;
	if (ffs_opts->minfree == -1)
		ffs_opts->minfree = MINFREE;
	if (ffs_opts->optimization == -1)
		ffs_opts->optimization = DEFAULTOPT;
	if (ffs_opts->maxcontig == -1)
		ffs_opts->maxcontig =
		    MAX(1, MIN(MAXPHYS, FFS_MAXBSIZE) / ffs_opts->bsize);
	/* XXX ondisk32 */
	if (ffs_opts->maxbpg == -1)
		ffs_opts->maxbpg = ffs_opts->bsize / sizeof(int32_t);
	if (ffs_opts->avgfilesize == -1)
		ffs_opts->avgfilesize = AVFILESIZ;
	if (ffs_opts->avgfpdir == -1)
		ffs_opts->avgfpdir = AFPDIR;

		/* calculate size of tree */
	traverse_tree(root, fsopts);
	fsopts->inodes += ROOTINO;		/* include first two inodes */

	if (debug & DEBUG_FS_VALIDATE)
		printf("ffs_validate: size of tree: %lld bytes, %lld inodes\n",
		    (long long)fsopts->size, (long long)fsopts->inodes);

		/* add requested slop */
	fsopts->size += fsopts->freeblocks;
	fsopts->inodes += fsopts->freefiles;
	if (fsopts->freefilepc > 0)
		fsopts->inodes =
		    fsopts->inodes * (100 + fsopts->freefilepc) / 100;
	if (fsopts->freeblockpc > 0)
		fsopts->size =
		    fsopts->size * (100 + fsopts->freeblockpc) / 100;

		/* add space needed for superblocks */
	/*
	 * The old SBOFF (SBLOCK_UFS1) is used here because makefs is
	 * typically used for small filesystems where space matters.
	 * XXX make this an option.
	 */
	fsopts->size += (SBLOCK_UFS1 + SBLOCKSIZE) * ncg;
		/* add space needed to store inodes, x3 for blockmaps, etc */
	if (ffs_opts->version == 1)
		fsopts->size += ncg * DINODE1_SIZE *
		    roundup(fsopts->inodes / ncg, 
			ffs_opts->bsize / DINODE1_SIZE);
	else
		fsopts->size += ncg * DINODE2_SIZE *
		    roundup(fsopts->inodes / ncg, 
			ffs_opts->bsize / DINODE2_SIZE);

		/* add minfree */
	if (ffs_opts->minfree > 0)
		fsopts->size =
		    fsopts->size * (100 + ffs_opts->minfree) / 100;
	/*
	 * XXX	any other fs slop to add, such as csum's, bitmaps, etc ??
	 */

	if (fsopts->size < fsopts->minsize)	/* ensure meets minimum size */
		fsopts->size = fsopts->minsize;

		/* round up to the next block */
	fsopts->size = roundup(fsopts->size, ffs_opts->bsize);

		/* calculate density if necessary */
	if (ffs_opts->density == -1)
		ffs_opts->density = fsopts->size / fsopts->inodes + 1;

	if (debug & DEBUG_FS_VALIDATE) {
		printf("ffs_validate: after defaults set:\n");
		ffs_dump_fsinfo(fsopts);
		printf("ffs_validate: dir %s; %lld bytes, %lld inodes\n",
		    dir, (long long)fsopts->size, (long long)fsopts->inodes);
	}

		/* now check calculated sizes vs requested sizes */
	if (fsopts->maxsize > 0 && fsopts->size > fsopts->maxsize) {
		errx(1, "`%s' size of %lld is larger than the maxsize of %lld.",
		    dir, (long long)fsopts->size, (long long)fsopts->maxsize);
	}

	/* XXX: missing some options */
	printf("newfs -F -s %lld -n %lld -b %lld -f %lld -m 0 %s\n",
	    (long long)(fsopts->size >> DEV_BSHIFT) + 1,
	    (long long)fsopts->inodes,
	    (long long)ffs_opts->bsize, (long long)ffs_opts->fsize,
	    image);
}


static void
ffs_dump_fsinfo(fsinfo_t *f)
{

	ffs_opt_t	*fs = f->fs_specific;

	printf("fsopts at %p\n", f);

	printf("\tsize %lld, inodes %lld, curinode %u\n",
	    (long long)f->size, (long long)f->inodes, f->curinode);

	printf("\tminsize %lld, maxsize %lld\n",
	    (long long)f->minsize, (long long)f->maxsize);
	printf("\tfree files %lld, freefile %% %d\n",
	    (long long)f->freefiles, f->freefilepc);
	printf("\tfree blocks %lld, freeblock %% %d\n",
	    (long long)f->freeblocks, f->freeblockpc);
	printf("\tneedswap %d, sectorsize %d\n", f->needswap, f->sectorsize);

	printf("\tbsize %d, fsize %d, cpg %d, density %d\n",
	    fs->bsize, fs->fsize, fs->cpg, fs->density);
	printf("\tnsectors %d, rpm %d, minfree %d\n",
	    fs->nsectors, fs->rpm, fs->minfree);
	printf("\tmaxcontig %d, maxbpg %d\n",
	    fs->maxcontig, fs->maxbpg);
	printf("\toptimization %s\n",
	    fs->optimization == FS_OPTSPACE ? "space" : "time");
}

#define	ADDDIRENT(e) do {						\
	tmpdir.d_namlen = strlen((e));					\
	this = DIRSIZ(0, &tmpdir, 0);					\
	if (debug & DEBUG_FS_SIZE_DIR_ADD_DIRENT)			\
		printf("ADDDIRENT: was: %s (%d) this %d cur %d\n",	\
		    e, tmpdir.d_namlen, this, curdirsize);		\
	if (this + curdirsize > roundup(curdirsize, DIRBLKSIZ))		\
		curdirsize = roundup(curdirsize, DIRBLKSIZ);		\
	curdirsize += this;						\
	if (debug & DEBUG_FS_SIZE_DIR_ADD_DIRENT)			\
		printf("ADDDIRENT: now: %s (%d) this %d cur %d\n",	\
		    e, tmpdir.d_namlen, this, curdirsize);		\
} while (0);

	/*
	 * XXX	this needs to take into account extra space consumed
	 *	by indirect blocks, etc.
	 */
#define	ADDSIZE(x) do {							\
	fsopts->size += roundup((x), ffs_opts->fsize);			\
} while (0);

static void
traverse_tree(fsnode *root, fsinfo_t *fsopts)
{
	struct direct	tmpdir;
	fsnode *	node;
	int		curdirsize, this;
	ffs_opt_t	*ffs_opts = fsopts->fs_specific;

	/* node may be NULL (empty directory) */
	assert(fsopts != NULL);
	assert(ffs_opts != NULL);

	if (debug & DEBUG_FS_SIZE_DIR)
		printf("ffs_size_dir: entry: bytes %lld inodes %lld\n",
		    (long long)fsopts->size, (long long)fsopts->inodes);

	curdirsize = 0;
	for (node = root; node != NULL; node = node->next) {
		ADDDIRENT(node->name);
		if (node == root) {			/* we're at "." */
			assert(strcmp(node->name, ".") == 0);
			ADDDIRENT("..");
		} else if ((node->inode->flags & FI_SIZED) == 0) {
				/* don't count duplicate names */
			node->inode->flags |= FI_SIZED;
			if (debug & DEBUG_FS_SIZE_DIR_NODE)
				printf("ffs_size_dir: `%s' size %lld\n",
				    node->name,
				    (long long)node->inode->st.st_size);
			fsopts->inodes++;
			if (node->type == S_IFREG)
				ADDSIZE(node->inode->st.st_size);
			if (node->type == S_IFLNK) {
				int	slen;

				slen = strlen(node->symlink) + 1;
				if (slen >= (ffs_opts->version == 1 ?
						MAXSYMLINKLEN_UFS1 :
						MAXSYMLINKLEN_UFS2))
					ADDSIZE(slen);
			}
		}
		if (node->type == S_IFDIR)
			traverse_tree(node->child, fsopts);
	}
	ADDSIZE(curdirsize);

	if (debug & DEBUG_FS_SIZE_DIR)
		printf("ffs_size_dir: exit: size %lld inodes %lld\n",
		    (long long)fsopts->size, (long long)fsopts->inodes);
}
