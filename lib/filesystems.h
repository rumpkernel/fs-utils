/* $NetBSD: filesystems.h,v 1.2 2009/12/13 20:57:25 pooka Exp $ */

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
#ifndef _FILESYSTEMS_H_
#define _FILESYSTEMS_H_

/*
 * To add a file system:
 * - add the include for the <fs>_args struct
 * - add the include for the mount_<fs>_parseargs function
 * - add the struct in the union
 * - add the file system in the fslist array
 */

/* get the definition of struct <fs>_args */
#ifdef __NetBSD__
#include <isofs/cd9660/cd9660_mount.h>
#include <fs/efs/efs_mount.h>
#include <ufs/ufs/ufsmount.h> /* ext2fs, ffs, lfs */
#include <fs/hfs/hfs.h>
#include <msdosfs/msdosfsmount.h>
#include <ntfs/ntfsmount.h>
#include <fs/sysvbfs/sysvbfs_args.h>
#include <fs/tmpfs/tmpfs_args.h>
#include <fs/udf/udf_mount.h>
#include <fs/v7fs/v7fs_args.h>
#include <nfs/nfsmount.h>
#ifdef WITH_SMBFS
#include <smbfs.h>
#include <netsmb/smb_lib.h>
#endif
#else
#include "nb_fs.h"
#endif

/* get the definition of mount_<fs>_parseargs */
#include "mount_cd9660.h"
#include "mount_efs.h"
#include "mount_ext2fs.h"
#include "mount_ffs.h"
#include "mount_hfs.h"
#include "mount_lfs.h"
#include "mount_msdos.h"
#include "mount_ntfs.h"
#include "mount_sysvbfs.h"
#include "mount_tmpfs.h"
#include "mount_udf.h"
#include "mount_kernfs.h"
#include "mount_v7fs.h"
#include "mount_nfs.h"
#ifdef WITH_SYSPUFFS
#include "rump_syspuffs.h"
#endif
#ifdef WITH_SMBFS
#include "mount_smbfs.h"
#endif

typedef int (*parseargs_fp)(int, char **, void *, int *, char *, char *);

typedef struct fsu_fs_s {
	const char *fs_name;
	void *fs_args;
	unsigned int fs_args_size;
	parseargs_fp fs_parseargs;
	unsigned int fs_flags;
#define FS_NO_AUTO (1)
} fsu_fs_t;

#define FS(a, b, c, d, e)						      \
	{ MOUNT_##a, &_args.args_##b, sizeof(struct c),			      \
		    (parseargs_fp)mount_##d##_parseargs, e }

union fsu_args {
	struct iso_args args_cd9660;
	struct efs_args args_efs;
	struct ufs_args args_ufs; /* ext2fs, ffs, lfs, ufs */
	struct hfs_args args_hfs;
	struct msdosfs_args args_msdos;
	struct ntfs_args args_ntfs;
	struct sysvbfs_args args_sysvbfs;
	struct tmpfs_args args_tmpfs;
	struct udf_args args_udf;
	struct v7fs_args args_v7fs;
	struct nfs_args args_nfs;
#ifdef WITH_SYSPUFFS
	struct syspuffs_args args_syspuffs;
#endif
#ifdef WITH_SMBFS
	struct smbfs_args args_smbfs;
#endif
} _args;

#define FSU_ISDISKFS(fsufs) ((fsufs->fs_flags & FS_NO_AUTO) == 0)

/* supported file systems */
fsu_fs_t fslist[] = {
	/* MOUNT_	args_	struct		parseargs	autodetect */
	FS(CD9660,	cd9660,	iso_args,	cd9660,		0),
	FS(EXT2FS,	ufs,	ufs_args,	ext2fs,		0),
	FS(FFS,		ufs,	ufs_args,	ffs,		0),
#ifndef __linux__
	FS(HFS,		hfs,	hfs_args,	hfs,		0),
#endif
	FS(LFS,		ufs,	ufs_args,	lfs,		0),
	FS(MSDOS,	msdos,	msdosfs_args,	msdos,		0),
	FS(NTFS,	ntfs,	ntfs_args,	ntfs,		0),
	FS(UDF,		udf,	udf_args,	udf,		0),
	FS(EFS,		efs,	efs_args,	efs,		0),
	FS(SYSVBFS,	sysvbfs, sysvbfs_args,	sysvbfs,	0),
	FS(V7FS,	v7fs, v7fs_args,	v7fs,		0),
	FS(TMPFS,	tmpfs,	tmpfs_args,	tmpfs,		FS_NO_AUTO),
	FS(NFS,		nfs,	nfs_args,	nfs,		FS_NO_AUTO),
#ifdef WITH_SYSPUFFS
	FS(PUFFS,	syspuffs, syspuffs_args, syspuffs,	FS_NO_AUTO),
#endif
#ifdef WITH_SMBFS
	FS(SMBFS,	smbfs,	smbfs_args,	smbfs,		FS_NO_AUTO),
#endif
	{ MOUNT_KERNFS, NULL, 0, (parseargs_fp)mount_kernfs_parseargs, FS_NO_AUTO },
	{ NULL,		NULL,			0,  NULL, 0 }
};

#endif
