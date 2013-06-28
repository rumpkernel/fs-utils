/*	$NetBSD: df.c,v 1.90 2012/01/07 18:45:13 christos Exp $ */

/*
 * Copyright (c) 1980, 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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

#include "fs-utils.h"

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fsu_utils.h>
#include <fsu_mount.h>
#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "fsu_compat.h"

/* from sys/statvfs.h */
#define	NB_VFS_NAMELEN	32
#define	NB_VFS_MNAMELEN	1024
struct nbstatvfs {
	unsigned long	f_flag;
	unsigned long	f_bsize;
	unsigned long	f_frsize;
	unsigned long	f_iosize;

	uint64_t	f_blocks;

	uint64_t	f_bfree;
	uint64_t	f_bavail;
	uint64_t	f_bresvd;

	uint64_t	f_files;
	uint64_t	f_ffree;
	uint64_t	f_favail;
	uint64_t	f_fresvd;

	uint64_t  	f_syncreads;
	uint64_t  	f_syncwrites;

	uint64_t  	f_asyncreads;
	uint64_t  	f_asyncwrites;

	uint32_t	f_notused[2];
	unsigned long	f_fsid;
	unsigned long	f_namemax;
	uint32_t	f_owner;

	uint32_t	f_spare[4];

	char	f_fstypename[NB_VFS_NAMELEN];
	char	f_mntonname[NB_VFS_MNAMELEN];
	char	f_mntfromname[NB_VFS_MNAMELEN];
};

static void	 prtstat(struct nbstatvfs *, int);
__dead static void usage(void);
static void	 prthumanval(int64_t, const char *);
static void	 prthuman(struct nbstatvfs *, int64_t, int64_t);

static int	 gflag, hflag, iflag, nflag, Pflag;
static long	 usize;
extern int rump_i_know_what_i_am_doing_with_sysents;

int
main(int argc, char *argv[])
{
	struct nbstatvfs mntbuf;
	int ch;

	setprogname(argv[0]);
	(void)setlocale(LC_ALL, "");

	if (fsu_mount(&argc, &argv, MOUNT_READONLY) != 0)
		usage();

	rump_i_know_what_i_am_doing_with_sysents = 1;
	rump_pub_lwproc_sysent_usenative();

	while ((ch = getopt(argc, argv, "aGghiklmnPt:")) != -1)
		switch (ch) {
		case 'a':
			/* ignored since it doesn't really make sense here */
			break;
		case 'g':
			hflag = 0;
			usize = 1024 * 1024 * 1024;
			break;
		case 'G':
			gflag = 1;
			break;
		case 'h':
			hflag = 1;
			usize = 0;
			break;
		case 'i':
			iflag = 1;
			break;
		case 'k':
			hflag = 0;
			usize = 1024;
			break;
		case 'l':
			/* ignored since it doesn't really make sense here */
			break;
		case 'm':
			hflag = 0;
			usize = 1024 * 1024;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'P':
			Pflag = 1;
			break;
		case 't':
			/* ignored */
			break;
		case '?':
		default:
			usage();
		}

	if (gflag && (Pflag || iflag))
		errx(EXIT_FAILURE,
		    "only one of -G and -P or -i may be specified");
	if (Pflag && iflag)
		errx(EXIT_FAILURE,
		    "only one of -P and -i may be specified");
#if 0
	/*
	 * The block size cannot be checked until after getbsize() is called.
	 */
	if (Pflag && (hflag || (usize != 1024 && usize != 512)))
		errx(EXIT_FAILURE,
		    "non-standard block size incompatible with -P");
#endif
	argc -= optind;
	argv += optind;

	if (rump_sys_statvfs1("/",
	    (struct statvfs *)&mntbuf, RUMP_MNT_WAIT) == -1)
		errx(1, "statvfs failed: %d", errno);
	prtstat(&mntbuf, strlen(mntbuf.f_mntfromname));
	return 0;
}

static void
prthumanval(int64_t bytes, const char *pad)
{
	char buf[6];

	(void)humanize_number(buf, sizeof(buf) - (bytes < 0 ? 0 : 1),
	    bytes, "", HN_AUTOSCALE,
	    HN_B | HN_NOSPACE | HN_DECIMAL);

	(void)printf("%s %6s", pad, buf);
}

static void
prthuman(struct nbstatvfs *sfsp, int64_t used, int64_t bavail)
{

	prthumanval((int64_t)(sfsp->f_blocks * sfsp->f_frsize), "   ");
	prthumanval((int64_t)(used * sfsp->f_frsize), "    ");
	prthumanval((int64_t)(bavail * sfsp->f_frsize), "    ");
}

/*
 * Convert statvfs returned filesystem size into BLOCKSIZE units.
 * Attempts to avoid overflow for large filesystems.
 */
#define fsbtoblk(num, fsbs, bs)					\
	(((fsbs) != 0 && (uint64_t)(fsbs) < (uint64_t)(bs)) ?	\
	    (int64_t)(num) / (int64_t)((bs) / (fsbs)) :		\
	    (int64_t)(num) * (int64_t)((fsbs) / (bs)))

/*
 * Print out status about a filesystem.
 */
static void
prtstat(struct nbstatvfs *sfsp, int maxwidth)
{
	static long blocksize;
	static int headerlen, timesthrough;
	static const char *header;
	static const char full[] = "100";
	static const char empty[] = "  0";
	int64_t used, availblks, inodes;
	int64_t bavail;
	char pb[64];

	if (gflag) {
		/*
		 * From SunOS-5.6:
		 *
		 * /var               (/dev/dsk/c0t0d0s3 ):         8192 block size          1024 frag size
		 *   984242 total blocks     860692 free blocks   859708 available         249984 total files
		 *   248691 free files      8388611 filesys id
		 *      ufs fstype       0x00000004 flag             255 filename length
		 *
		 */
		(void)printf("%10s (%-12s): %7ld block size %12ld frag size\n",
		    sfsp->f_mntonname, sfsp->f_mntfromname,
		    sfsp->f_iosize,	/* On UFS/FFS systems this is
					 * also called the "optimal
					 * transfer block size" but it
					 * is of course the file
					 * system's block size too.
					 */
		    sfsp->f_bsize);	/* not so surprisingly the
					 * "fundamental file system
					 * block size" is the frag
					 * size.
					 */
		(void)printf("%10" PRId64 " total blocks %10" PRId64
		    " free blocks  %10" PRId64 " available\n",
		    (uint64_t)sfsp->f_blocks, (uint64_t)sfsp->f_bfree,
		    (uint64_t)sfsp->f_bavail);
		(void)printf("%10" PRId64 " total files  %10" PRId64
		    " free files %12lx filesys id\n",
		    (uint64_t)sfsp->f_ffree, (uint64_t)sfsp->f_files,
		    sfsp->f_fsid);
		(void)printf("%10s fstype  %#15lx flag  %17ld filename "
		    "length\n", sfsp->f_fstypename, sfsp->f_flag,
		    sfsp->f_namemax);
		(void)printf("%10lu owner %17" PRId64 " syncwrites %12" PRId64
		    " asyncwrites\n\n", (unsigned long)sfsp->f_owner,
		    sfsp->f_syncwrites, sfsp->f_asyncwrites);

		/*
		 * a concession by the structured programming police to the
		 * indentation police....
		 */
		return;
	}
	if (maxwidth < 12)
		maxwidth = 12;
	if (++timesthrough == 1) {
		switch (blocksize = usize) {
		case 1024:
			header = Pflag ? "1024-blocks" : "1K-blocks";
			headerlen = (int)strlen(header);
			break;
		case 1024 * 1024:
			header = "1M-blocks";
			headerlen = (int)strlen(header);
			break;
		case 1024 * 1024 * 1024:
			header = "1G-blocks";
			headerlen = (int)strlen(header);
			break;
		default:
			if (hflag) {
				header = "Size";
				headerlen = (int)strlen(header);
			} else
				header = getbsize(&headerlen, &blocksize);
			break;
		}
		if (Pflag) {
			/*
			 * either:
			 *  "Filesystem 1024-blocks Used Available Capacity Mounted on\n"
			 * or:
			 *  "Filesystem 512-blocks Used Available Capacity Mounted on\n"
			 */
			if (blocksize != 1024 && blocksize != 512)
				errx(EXIT_FAILURE,
				    "non-standard block size incompatible with -P");
			(void)printf("Filesystem %s Used Available Capacity "
			    "Mounted on\n", header);
		} else {
			(void)printf("%-*.*s %s       Used      Avail %%Cap",
			    maxwidth - (headerlen - 10),
			    maxwidth - (headerlen - 10),
			    "Filesystem", header);
			if (iflag)
				(void)printf("    iUsed   iAvail %%iCap");
			(void)printf(" Mounted on\n");
		}
	}
	used = sfsp->f_blocks - sfsp->f_bfree;
	bavail = sfsp->f_bfree - sfsp->f_bresvd;
	availblks = bavail + used;
	if (Pflag) {
		assert(hflag == 0);
		assert(blocksize > 0);
		/*
		 * "%s %d %d %d %s %s\n", <file system name>, <total space>,
		 * <space used>, <space free>, <percentage used>,
		 * <file system root>
		 */
		(void)printf("%s %" PRId64 " %" PRId64 " %" PRId64 " %s%% %s\n",
		    sfsp->f_mntfromname,
		    fsbtoblk(sfsp->f_blocks, sfsp->f_frsize, blocksize),
		    fsbtoblk(used, sfsp->f_frsize, blocksize),
		    fsbtoblk(bavail, sfsp->f_frsize, blocksize),
		    availblks == 0 ? full : strspct(pb, sizeof(pb), used,
		    availblks, 0), sfsp->f_mntonname);
		/*
		 * another concession by the structured programming police to
		 * the indentation police....
		 *
		 * Note iflag cannot be set when Pflag is set.
		 */
		return;
	}

	(void)printf("%-*.*s ", maxwidth, maxwidth, sfsp->f_mntfromname);

	if (hflag)
		prthuman(sfsp, used, bavail);
	else
		(void)printf("%10" PRId64 " %10" PRId64 " %10" PRId64,
		    fsbtoblk(sfsp->f_blocks, sfsp->f_frsize, blocksize),
		    fsbtoblk(used, sfsp->f_frsize, blocksize),
		    fsbtoblk(bavail, sfsp->f_frsize, blocksize));
	(void)printf(" %3s%%",
	    availblks == 0 ? full :
	    strspct(pb, sizeof(pb), used, availblks, 0));
	if (iflag) {
		inodes = sfsp->f_files;
		used = inodes - sfsp->f_ffree;
		(void)printf(" %8jd %8jd %3s%%",
		    (intmax_t)used, (intmax_t)sfsp->f_ffree,
		    inodes == 0 ? (used == 0 ? empty : full) :
		    strspct(pb, sizeof(pb), used, inodes, 0));
	}
	(void)printf(" %s\n", sfsp->f_mntonname);
}

static void
usage(void)
{

	(void)fprintf(stderr,
	    "Usage: %s %s [-aGgln] [-hkm|-ihkm|-Pk] [-t type] [file | "
	    "file_system ...]\n",
	    getprogname(),
	    fsu_mount_usage());
	exit(1);
	/* NOTREACHED */
}
