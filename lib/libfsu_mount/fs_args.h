/* $NetBSD: fs_args.h,v 1.1 2009/11/05 14:02:42 stacktic Exp $ */

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
#if !defined(_FS_ARGS_H_) && !defined(__NetBSD__)
#define _FS_ARGS_H_

#include <stdint.h>

/* not the real export_args30 cf include/sys/mount.h */
struct export_args30 {
	uint8_t pad[108];
};

struct iso_args {
	const char	*fspec;
	struct	export_args30 _pad1;
	int	flags;
};

struct efs_args {
	char   *fspec;
	int	version;
};

struct ufs_args {
	char	*fspec;
};

struct hfs_args {
	char *fspec;
};

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

struct ntfs_args {
	char	*fspec;
	struct	export_args30 _pad1;
	uint32_t	uid;
	uint32_t	gid;
	uint32_t	mode;
	u_long	flag;
};

struct sysvbfs_args {
	char	*fspec;
};

struct tmpfs_args {
	int			ta_version;
	uint64_t			ta_nodes_max;
	uint64_t			ta_size_max;
	uint32_t			ta_root_uid;
	uint32_t			ta_root_gid;
	uint32_t			ta_root_mode;
};

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

#endif
