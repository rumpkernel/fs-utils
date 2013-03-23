/* $NetBSD: nb_fs.h,v 1.1 2009/11/05 14:02:42 stacktic Exp $ */

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
#if !defined(_NB_FS_H_) && !defined(__NetBSD__)
#define _NB_FS_H_

#include <stdint.h>

#define	MOUNT_FFS	"ffs"		/* UNIX "Fast" Filesystem */
#define	MOUNT_UFS	MOUNT_FFS	/* for compatibility */
#define	MOUNT_NFS	"nfs"		/* Network Filesystem */
#define	MOUNT_MSDOS	"msdos"		/* MSDOS Filesystem */
#define	MOUNT_LFS	"lfs"		/* Log-based Filesystem */
#define	MOUNT_CD9660	"cd9660"	/* ISO9660 (aka CDROM) Filesystem */
#define	MOUNT_EXT2FS	"ext2fs"	/* Second Extended Filesystem */
#define	MOUNT_NTFS	"ntfs"		/* Windows/NT Filesystem */
#define	MOUNT_SMBFS	"smbfs"		/* CIFS (SMB) */
#define	MOUNT_TMPFS	"tmpfs"		/* Efficient memory file-system */
#define MOUNT_UDF	"udf"		/* UDF CD/DVD filesystem */
#define	MOUNT_SYSVBFS	"sysvbfs"	/* System V Boot Filesystem */
#define MOUNT_PUFFS	"puffs"		/* Pass-to-Userspace filesystem */
#define MOUNT_HFS	"hfs"		/* Apple HFS+ Filesystem */
#define MOUNT_EFS	"efs"		/* SGI's Extent Filesystem */
#define MOUNT_KERNFS	"kernfs"	/* Kernel Information Filesystem */
#define MOUNT_V7FS	"v7fs"		/* 7th Edition of Unix Filesystem */

/* not the real export_args30 cf include/sys/mount.h */
struct export_args30 {
	uint8_t pad[88];
        void *pad2[5];
};

/* cd9660_mount.h */
struct iso_args {
	const char	*fspec;
	struct	export_args30 _pad1;
	int	flags;
};
#define	ISOFSMNT_NORRIP		0x00000001
#define	ISOFSMNT_GENS		0x00000002
#define	ISOFSMNT_EXTATT		0x00000004
#define	ISOFSMNT_NOJOLIET	0x00000008
#define	ISOFSMNT_NOCASETRANS	0x00000010
#define	ISOFSMNT_RRCASEINS	0x00000020
#define ISOFSMNT_BITS "\177\20" \
    "b\00norrip\0b\01gens\0b\02extatt\0b\03nojoliet\0" \
    "b\04nocasetrans\0b\05rrcaseins\0"

/* efs_mount.h */
struct efs_args {
	char   *fspec;
	int	version;
};
#define EFS_MNT_VERSION 0

struct ufs_args {
	char	*fspec;
};

/* hfs.h */
struct hfs_args {
	char *fspec;
};
#define HFS_DEFAULT_UID	0
#define HFS_DEFAULT_GID	0
#define HFS_DEFAULT_DIR_MODE	0755
#define HFS_DEFAULT_FILE_MODE	0755

/* msdosfsmount.h */
struct msdosfs_args {
	char	*fspec;
	struct	export_args30 _pad1;
	uint32_t	uid;
	uint32_t	gid;
	uint32_t  mask;
	int	flags;
	int	version;
	uint32_t  dirmask;
	int	gmtoff;
};
#define MSDOSFSMNT_VERSION	3
#define	MSDOSFSMNT_SHORTNAME	1
#define	MSDOSFSMNT_LONGNAME	2
#define	MSDOSFSMNT_NOWIN95	4
#define	MSDOSFSMNT_GEMDOSFS	8
#define MSDOSFSMNT_VERSIONED	16
#define	MSDOSFSMNT_MNTOPT \
	(MSDOSFSMNT_SHORTNAME|MSDOSFSMNT_LONGNAME|MSDOSFSMNT_NOWIN95 \
	 |MSDOSFSMNT_GEMDOSFS|MSDOSFSMNT_VERSIONED)
#define	MSDOSFSMNT_RONLY	0x80000000
#define	MSDOSFSMNT_WAITONFAT	0x40000000
#define	MSDOSFS_FATMIRROR	0x20000000
#define MSDOSFSMNT_BITS "\177\20" \
 "b\00shortname\0b\01longname\0b\02nowin95\0b\03gemdosfs\0b\04mntversioned\0" \
 "b\037ronly\0b\036waitonfat\0b\035fatmirror\0"

/* ntfsmount.h */
struct ntfs_args {
	char	*fspec;
	struct	export_args30 _pad1;
	uint32_t	uid;
	uint32_t	gid;
	uint32_t	mode;
	u_long	flag;
};
#define	NTFS_MFLAG_CASEINS	0x00000001
#define	NTFS_MFLAG_ALLNAMES	0x00000002
#define NTFS_MFLAG_BITS	"\177\20" \
    "b\00caseins\0b\01allnames\0"

/* sysvbfs_args.h */
struct sysvbfs_args {
	char	*fspec;
};

/* tmpfs_args.h */
struct tmpfs_args {
	int			ta_version;
	uint64_t			ta_nodes_max;
	uint64_t			ta_size_max;
	uint32_t			ta_root_uid;
	uint32_t			ta_root_gid;
	uint32_t			ta_root_mode;
};
#define TMPFS_ARGS_VERSION	1

/* udf_mount.h */
struct udf_args {
	uint32_t	 version;
	char		*fspec;
	int32_t		 sessionnr;
	uint32_t	 udfmflags;
	int32_t		 gmtoff;
	uint32_t		 anon_uid;
	uint32_t		 anon_gid;
	uint32_t		 nobody_uid;
	uint32_t		 nobody_gid;
	uint32_t	 sector_size;
	uint8_t	 reserved[32];
};
#define UDFMNT_VERSION	1
#define UDFMNT_CLOSESESSION	0x00000001
#define UDFMNT_BITS "\20\1CLOSESESSION"

#ifdef WITH_NFS
struct nfs_args {
	int		version;
	struct sockaddr	*addr;
	int		addrlen;
	int		sotype;
	int		proto;
	u_char		*fh;
	int		fhsize;
	int		flags;
	int		wsize;
	int		rsize;
	int		readdirsize;
	int		timeo;
	int		retrans;
	int		maxgrouplist;
	int		readahead;
	int		leaseterm;
	int		deadthresh;
	char		*hostname;
};
#endif

/* syspuffs_args */
/* smbfs_args */

struct v7fs_args {
	char    *fspec;	/* blocks special holding the fs to mount */
	int endian;	/* target filesystem endian */
};
#endif
