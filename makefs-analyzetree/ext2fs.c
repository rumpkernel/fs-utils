/*	$NetBSD: ext2fs.c,v 1.2 2009/07/13 17:11:37 stacktic Exp $	*/

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
 *	@(#)ext2fs_alloc.c	8.19 (Berkeley) 7/13/95
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: ext2fs.c,v 1.2 2009/07/13 17:11:37 stacktic Exp $");
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
#include "ext2fs.h"

#if HAVE_STRUCT_STATVFS_F_IOSIZE && HAVE_FSTATVFS
#include <sys/statvfs.h>
#endif

#include <ufs/ext2fs/ext2fs.h>
#include <ufs/ext2fs/ext2fs_dinode.h>
#include <ufs/ext2fs/ext2fs_dir.h>
#define FFS_MAXBSIZE (1<<16)

#ifndef EXT2_REV0_DINODE_SIZE
#define EXT2_REV0_DINODE_SIZE (EXT2_DINODE_SIZE)
#endif

/*
 * Various file system defaults (cribbed from newfs(8)).
 */
#define	DFL_BLKSIZE		4096		/* block size */
#define	DFL_SECSIZE		512		/* sector size */
#define	DFL_CYLSPERGROUP	65536		/* cylinders per group */
#define	DFL_FRAGSPERINODE	4		/* fragments per inode */
#define	DFL_ROTDELAY		0		/* rotational delay */
#define	DFL_NRPOS		1		/* rotational positions */
#define	DFL_RPM			3600		/* rpm of disk */
#define	DFL_NSECTORS		64		/* # of sectors */
#define	DFL_NTRACKS		16		/* # of tracks */

static	void	ext2fs_dump_fsinfo(fsinfo_t *);
static	void	traverse_tree(fsnode *, fsinfo_t *);


	/* publically visible functions */

void
ext2fs_prep_opts(fsinfo_t *fsopts)
{
	ext2fs_opt_t *ext2fs_opts;

	if ((ext2fs_opts = calloc(1, sizeof(ext2fs_opt_t))) == NULL)
		err(1, "Allocating memory for ext2fs_options");

	fsopts->fs_specific = ext2fs_opts;

	ext2fs_opts->bsize= -1;
	ext2fs_opts->density= -1;
	ext2fs_opts->minfree= -1;
}

void
ext2fs_cleanup_opts(fsinfo_t *fsopts)
{
	if (fsopts->fs_specific)
		free(fsopts->fs_specific);
}

int
ext2fs_parse_opts(const char *option, fsinfo_t *fsopts)
{
	ext2fs_opt_t	*ext2fs_opts = fsopts->fs_specific;

	option_t ext2fs_options[] = {
		{ "bsize",	&ext2fs_opts->bsize,	1,	INT_MAX,
					"block size" },
		{ "density",	&ext2fs_opts->density,	1,	INT_MAX,
					"bytes per inode" },
		{ "minfree",	&ext2fs_opts->minfree,	0,	99,
					"minfree" },
		{ .name = NULL }
	};

	char	*var, *val;
	int	rv;

	assert(option != NULL);
	assert(fsopts != NULL);
	assert(ext2fs_opts != NULL);

	if (debug & DEBUG_FS_PARSE_OPTS)
		printf("ext2fs_parse_opts: got `%s'\n", option);

	if ((var = strdup(option)) == NULL)
		err(1, "Allocating memory for copy of option string");
	rv = 0;

	if ((val = strchr(var, '=')) == NULL) {
		warnx("Option `%s' doesn't contain a value", var);
		goto leave_ext2fs_parse_opts;
	}
	*val++ = '\0';

	rv = set_option(ext2fs_options, var, val);

 leave_ext2fs_parse_opts:
	if (var)
		free(var);
	return (rv);
}


void
ext2fs_makefs(const char *image, const char *dir, fsnode *root, fsinfo_t *fsopts)
{
	int32_t	ncg = 1; /* XXX */
	ext2fs_opt_t	*ext2fs_opts = fsopts->fs_specific;

	assert(image != NULL);
	assert(dir != NULL);
	assert(root != NULL);
	assert(fsopts != NULL);

	if (debug & DEBUG_FS_MAKEFS)
		printf("ext2fs_makefs: image %s directory %s root %p\n",
		    image, dir, root);

	assert(dir != NULL);
	assert(root != NULL);
	assert(fsopts != NULL);
	assert(ext2fs_opts != NULL);

	if (debug & DEBUG_FS_VALIDATE) {
		printf("ext2fs_validate: before defaults set:\n");
		ext2fs_dump_fsinfo(fsopts);
	}

		/* set FFS defaults */
	if (fsopts->sectorsize == -1)
		fsopts->sectorsize = DFL_SECSIZE;
	if (ext2fs_opts->bsize == -1)
		ext2fs_opts->bsize = DFL_BLKSIZE;
	if (ext2fs_opts->nsectors == -1)
		ext2fs_opts->nsectors = DFL_NSECTORS;
	if (ext2fs_opts->minfree == -1)
		ext2fs_opts->minfree = MINFREE;

		/* calculate size of tree */
	traverse_tree(root, fsopts);
	fsopts->inodes += EXT2_FIRSTINO; /* start from non-reserved inode */

	if (debug & DEBUG_FS_VALIDATE)
		printf("ext2fs_validate: size of tree: %lld bytes, %lld inodes\n",
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
	fsopts->size += SBSIZE * ncg;
	/* XXX: hardcode rev0 */
	fsopts->size += ncg * EXT2_REV0_DINODE_SIZE *
	    roundup(fsopts->inodes / ncg, 
			ext2fs_opts->bsize / EXT2_REV0_DINODE_SIZE);

		/* add minfree */
	if (ext2fs_opts->minfree > 0)
		fsopts->size =
		    fsopts->size * (100 + ext2fs_opts->minfree) / 100;
	/*
	 * XXX	any other fs slop to add, such as csum's, bitmaps, etc ??
	 */

	if (fsopts->size < fsopts->minsize)	/* ensure meets minimum size */
		fsopts->size = fsopts->minsize;

		/* round up to the next block */
	fsopts->size = roundup(fsopts->size, ext2fs_opts->bsize);

		/* calculate density if necessary */
	if (ext2fs_opts->density == -1)
		ext2fs_opts->density = fsopts->size / fsopts->inodes + 1;

	if (debug & DEBUG_FS_VALIDATE) {
		printf("ext2fs_validate: after defaults set:\n");
		ext2fs_dump_fsinfo(fsopts);
		printf("ext2fs_validate: dir %s; %lld bytes, %lld inodes\n",
		    dir, (long long)fsopts->size, (long long)fsopts->inodes);
	}

		/* now check calculated sizes vs requested sizes */
	if (fsopts->maxsize > 0 && fsopts->size > fsopts->maxsize) {
		errx(1, "`%s' size of %lld is larger than the maxsize of %lld.",
		    dir, (long long)fsopts->size, (long long)fsopts->maxsize);
	}

	/* XXX: missing some options */
	printf("newfs_ext2fs -F -s %lld -n %lld -b %lld -m 0 %s\n",
	    (long long)(fsopts->size >> DEV_BSHIFT) + 1,
	    (long long)fsopts->inodes,
	    (long long)ext2fs_opts->bsize,
	    image);
}


static void
ext2fs_dump_fsinfo(fsinfo_t *f)
{

	ext2fs_opt_t	*fs = f->fs_specific;

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

	printf("\tbsize %d\n", fs->bsize);
	printf("\tnsectors %d, minfree %d\n",
	    fs->nsectors, fs->minfree);
}

#define	ADDDIRENT(e2opts, e) do {					\
	tmpdir.e2d_namlen = strlen((e));				\
	this = EXT2FS_DIRSIZ(tmpdir.e2d_namlen);			\
	if (debug & DEBUG_FS_SIZE_DIR_ADD_DIRENT)			\
		printf("ADDDIRENT: was: %s (%d) this %d cur %d\n",	\
		    e, tmpdir.e2d_namlen, this, curdirsize);		\
	if (this + curdirsize > roundup(curdirsize, e2opts->bsize))	\
		curdirsize = roundup(curdirsize, e2opts->bsize);	\
	curdirsize += this;						\
	if (debug & DEBUG_FS_SIZE_DIR_ADD_DIRENT)			\
		printf("ADDDIRENT: now: %s (%d) this %d cur %d\n",	\
		    e, tmpdir.e2d_namlen, this, curdirsize);		\
} while (0);

	/*
	 * XXX	this needs to take into account extra space consumed
	 *	by indirect blocks, etc.
	 */
#define	ADDSIZE(x) do {							\
	fsopts->size += roundup((x), ext2fs_opts->bsize);			\
} while (0);

static void
traverse_tree(fsnode *root, fsinfo_t *fsopts)
{
	struct ext2fs_direct	tmpdir;
	fsnode *	node;
	int		curdirsize, this;
	ext2fs_opt_t	*ext2fs_opts = fsopts->fs_specific;

	/* node may be NULL (empty directory) */
	assert(fsopts != NULL);
	assert(ext2fs_opts != NULL);

	if (debug & DEBUG_FS_SIZE_DIR)
		printf("ext2fs_size_dir: entry: bytes %lld inodes %lld\n",
		    (long long)fsopts->size, (long long)fsopts->inodes);

	curdirsize = 0;
	for (node = root; node != NULL; node = node->next) {
		ADDDIRENT(ext2fs_opts, node->name);
		if (node == root) {			/* we're at "." */
			assert(strcmp(node->name, ".") == 0);
			ADDDIRENT(ext2fs_opts, "..");
		} else if ((node->inode->flags & FI_SIZED) == 0) {
				/* don't count duplicate names */
			node->inode->flags |= FI_SIZED;
			if (debug & DEBUG_FS_SIZE_DIR_NODE)
				printf("ext2fs_size_dir: `%s' size %lld\n",
				    node->name,
				    (long long)node->inode->st.st_size);
			fsopts->inodes++;
			if (node->type == S_IFREG)
				ADDSIZE(node->inode->st.st_size);
			if (node->type == S_IFLNK) {
				int	slen;

				slen = strlen(node->symlink) + 1;
				if (slen >= EXT2_MAXSYMLINKLEN)
					ADDSIZE(slen);
			}
		}
		if (node->type == S_IFDIR)
			traverse_tree(node->child, fsopts);
	}
	ADDSIZE(curdirsize);

	if (debug & DEBUG_FS_SIZE_DIR)
		printf("ext2fs_size_dir: exit: size %lld inodes %lld\n",
		    (long long)fsopts->size, (long long)fsopts->inodes);
}
