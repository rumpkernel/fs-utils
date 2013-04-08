/* $NetBSD: fsu_mount.c,v 1.20 2011/02/09 16:47:13 pooka Exp $ */

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

#include <sys/mount.h>
#include <sys/stat.h>

#if HAVE_NBCOMPAT_H
#include <nbcompat.h>
#endif

#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <fsu_utils.h>

#include "fsu_mount.h"

#include "filesystems.h"
#include "fsu_alias.h"

#define MOUNT_DIRECTORY "/mnt"

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef RUMP_LIBDIR
#define RUMP_LIBDIR "/usr/lib"
#endif

#ifndef __UNCONST
#define __UNCONST(a) ((char *)(unsigned long)(const char *)(a))
#endif

#ifdef __linux__
#define GETOPT_PREFIX "+"
#else
#define GETOPT_PREFIX
#endif

#define ADD_ARG(m, a) 					\
    do { 						\
	char *_tmp = (a);				\
	if ((_tmp) == NULL) { 				\
	    	warn(NULL);				\
		return -1;				\
	}						\
	(m)->mntd_argv[(m)->mntd_argc++] = (_tmp);	\
    } while (0/*CONSTCOND*/)
struct mount_data_s {
	fsu_fs_t *mntd_fs;
	char mntd_canon_dev[MAXPATHLEN];
	char mntd_canon_dir[MAXPATHLEN];
	char *mntd_fsdevice;
	int mntd_flags;
	int mntd_argc;
	char **mntd_argv;
	int mntd_argv_size;
};

static int mount_alias(struct fsu_fsalias_s *, char *, char *,
    struct mount_data_s *, int);
static int mount_fstype(fsu_fs_t *, const char *, char *, char *,
    char *, struct mount_data_s *, int);
static int fsu_load_fs(const char *);

static int mount_struct(_Bool, struct mount_data_s *);
extern int rump_i_know_what_i_am_doing_with_sysents;

/*
 * Tries to mount an image.
 * if the fstype is not given try every supported types.
 */
int
fsu_mount(int *argc, char **argv[], int mode)
{
	fsu_fs_t *fst;
	struct fsu_fsalias_s *alias;
	struct mount_data_s mntd;
	int idx, fflag, rv, verbose;
	int ch;
	char *mntopts, afsdev[MAXPATHLEN], *puffsexec, *specopts;
	char *tmp;
	char *fsdevice, *fstype;
	struct stat sb;
#ifdef WITH_SYSPUFFS
	const char options[] = GETOPT_PREFIX"f:o:p:s:t:v";
#else
	const char options[] = GETOPT_PREFIX"f:o:s:t:v";
#endif

	alias = NULL;
	fsdevice = fstype = mntopts = puffsexec = specopts = NULL;
	fst = NULL;
	verbose = fflag = 0;
	memset(&mntd, 0, sizeof(mntd));
	mntd.mntd_fsdevice = mntd.mntd_canon_dev;

	rump_init();
	/*
	 * [-o mnt_args] [-t fstype] [-p puffsexec] fsdevice
	 */
	while ((ch = getopt(*argc, *argv, options)) != -1) {
		switch (ch) {
		case 'f':
			if (fsdevice == NULL)
				fsdevice = optarg;
			fflag = 1;
			break;
		case 'o':
			if (mntopts == NULL)
				mntopts = optarg;
			break;
#ifdef WITH_SYSPUFFS
		case 'p':
			if (puffsexec == NULL) {
				puffsexec = optarg;
				for (fst = fslist; fst->fs_name != NULL; ++fst)
					if (fst->fs_name == MOUNT_PUFFS)
						break;

				if (fstype == NULL)
					fstype = MOUNT_PUFFS;
				else if (strcmp(fstype, MOUNT_PUFFS) != 0) {
					warnx("-p is only for puffs");
					return -1;
				}
			}
			break;
#endif
                case 's':
                        if (specopts == NULL)
                                specopts = optarg;
                        break;
		case 't':
			if (fstype == NULL)
				fstype = optarg;
			break;
		case 'v':
			++verbose;
			break;
		case '?':
		default:
			errno = EINVAL;
			return -1;
		}
	}
	idx = optind;
	optind = 1;
#ifdef HAVE_GETOPT_OPTRESET
	optreset = 1;
#endif
	if (mntopts == NULL)
		mntopts = getenv("FSU_MNTOPTS");

	if (mode == MOUNT_READONLY) {
		if (mntopts == NULL)
			mntopts = __UNCONST("ro");
		else {
			tmp = malloc(strlen(mntopts) + 4);
			if (tmp == NULL) {
				warn(NULL);
				return -1;
			}
			snprintf(tmp, strlen(mntopts) + 4, "%s,ro", mntopts);
			mntopts = tmp;
		}
	}

	if (fstype == NULL)
	    	fstype = getenv("FSU_TYPE");

	if (fstype != NULL && fst == NULL) {
		for (fst = fslist; fst->fs_name != NULL; ++fst)
			if (strcmp(fstype, fst->fs_name) == 0)
				break;

		if (fst->fs_name == NULL) {
			fprintf(stderr, "%s: filesystem not supported\n",
				fstype);
			return -1;
		}
	}

       	if (fsdevice == NULL) {
		fsdevice = getenv("FSU_DEVICE");
		if (fsdevice == NULL) {
			if (idx < *argc && strcmp((*argv)[idx], "--") != 0)
				fsdevice = (*argv)[idx++];
			else {
				errno = 0;
				return -1;
			}
		}
	}

	rv = 0;
	if (!fflag) {
		rv = -1;
		build_alias_list();
		alias = get_alias(fsdevice);
		if (alias != NULL)
			rv = mount_alias(alias, mntopts, specopts, &mntd, verbose);
		free_alias_list();
	}
	if (fflag || alias == NULL) {
		if (realpath(fsdevice, afsdev) != NULL)
			fsdevice = afsdev;
		if (stat(fsdevice, &sb) == 0 && S_ISREG(sb.st_mode)) {
			rv = rump_pub_etfs_register("/dev/rumptest", fsdevice,
					RUMP_ETFS_BLK);
			mntd.mntd_fsdevice = fsdevice;
			fsdevice = strdup("/dev/rumptest");
		}
		rv = mount_fstype(fst, fsdevice, mntopts, puffsexec, specopts,
				&mntd, verbose);
	}

	free(mntd.mntd_argv);
	mntd.mntd_argv = NULL;
	mntd.mntd_argv_size = 0;

	/* Remove the arguments used by fsu_mount and reset getopt*/
	if ((*argv)[idx] != NULL && strcmp((*argv)[idx], "--") == 0)
		++idx;

	if (--idx > 0) {
		(*argv)[idx] = (*argv)[0];
		*argv += idx;
		*argc -= idx;
		optind = 1;
#ifdef HAVE_GETOPT_OPTRESET
		optreset = 1;
#endif
	}

	optind = 1;
#ifdef HAVE_GETOPT_OPTRESET
	optreset = 1;
#endif
	return rv;
}

static int
mount_fstype(fsu_fs_t *fs, const char *fsdev, char *mntopts, char *puffsexec,
    char *specopts, struct mount_data_s *mntdp, int verbose)
{
	int argvlen;

	mntdp->mntd_fs = fs;
	mntdp->mntd_argc = mntdp->mntd_flags = 0;

        argvlen = 7;
        if (specopts != NULL)
                argvlen += fsu_str2argc(specopts);

	if (argvlen > mntdp->mntd_argv_size) {
	    	char **tmp;

		tmp = realloc(mntdp->mntd_argv, argvlen * sizeof(char *));
		if (tmp == NULL) {
		    	if (mntdp->mntd_argv != NULL)
				free(mntdp->mntd_argv);
			return -1;
		}
		mntdp->mntd_argv = tmp;
		mntdp->mntd_argv_size = argvlen;
	}

	/* setting up the argv array */
	ADD_ARG(mntdp, __UNCONST(getprogname()));

#ifdef WITH_SYSPUFFS
	if (puffsexec != NULL && fs->fs_name == MOUNT_PUFFS)
                ADD_ARG(mntdp, puffsexec);
#endif

	if (mntopts != NULL) {
		ADD_ARG(mntdp, __UNCONST("-o"));
		ADD_ARG(mntdp, mntopts);
	}
        if (specopts != NULL) {
                int tmpargc;

                fsu_str2arg(specopts, &tmpargc,
                    mntdp->mntd_argv + mntdp->mntd_argc, argvlen - 6);
                mntdp->mntd_argc += tmpargc;
        }
	ADD_ARG(mntdp, __UNCONST(fsdev));
	ADD_ARG(mntdp, __UNCONST("/"));
	mntdp->mntd_argv[mntdp->mntd_argc] = NULL;

	/* filesystem given */
	if (fs != NULL)
                return mount_struct(verbose, mntdp);

	/* filesystem not given (auto detection) */
	for (fs = fslist; fs->fs_name != NULL; ++fs) {
		if (verbose)
			printf("Trying with fs %s\n", fs->fs_name);
		if (fs->fs_flags & FS_NO_AUTO)
			continue;
		mntdp->mntd_flags = 0;
                mntdp->mntd_fs = fs;
                if (mount_struct(verbose > 1, mntdp) == 0)
			return 0;
	}
	return -1;
}

static int
mount_alias(struct fsu_fsalias_s *al, char *mntopts, char *specopts,
    struct mount_data_s *mntdp, int verbose)
{
	fsu_fs_t *cur;
	int argvlen;

	mntdp->mntd_argc = mntdp->mntd_flags = 0;

        argvlen = 9;
        if (specopts != NULL)
                argvlen += fsu_str2argc(specopts);

	if (argvlen > mntdp->mntd_argv_size) {
	    	char **tmp;

		tmp = realloc(mntdp->mntd_argv, argvlen * sizeof(char *));
		if (tmp == NULL) {
		    	free(mntdp->mntd_argv);
			return -1;
		}
		mntdp->mntd_argv = tmp;
		mntdp->mntd_argv_size = argvlen;
	}

	ADD_ARG(mntdp, __UNCONST(getprogname()));

#ifdef WITH_SYSPUFFS
	if (al->fsa_puffsexec != NULL)
		ADD_ARG(mntdp, al->fsa_puffsexec);
#endif

	if (al->fsa_mntopt != NULL) {
		ADD_ARG(mntdp, __UNCONST("-o"));
		ADD_ARG(mntdp, al->fsa_mntopt);
		setenv("FSU_MNTOPTS", al->fsa_mntopt, 1);
	}
	if (mntopts != NULL) {
		ADD_ARG(mntdp, __UNCONST("-o"));
		ADD_ARG(mntdp, mntopts);
		setenv("FSU_MNTOPTS", mntopts, 1);
	}
        if (specopts != NULL) {
                int tmpargc;

                fsu_str2arg(specopts, &tmpargc,
                    mntdp->mntd_argv + mntdp->mntd_argc, argvlen - 8);
                mntdp->mntd_argc += tmpargc;
        }
	ADD_ARG(mntdp, al->fsa_path);
	ADD_ARG(mntdp, __UNCONST("/"));
	mntdp->mntd_argv[mntdp->mntd_argc] = NULL;

	for (cur = fslist; cur->fs_name != NULL; ++cur)
		if (strcmp(cur->fs_name, al->fsa_type) == 0)
			break;

	if (cur->fs_name == NULL)
                return -1;

        mntdp->mntd_fs = cur;

	return mount_struct(verbose, mntdp);
}

static int
mount_struct(_Bool verbose, struct mount_data_s *mntdp)
{
        fsu_fs_t *fs;
        int rv;

        fs = mntdp->mntd_fs;

        rv = fs->fs_parseargs(mntdp->mntd_argc, mntdp->mntd_argv, fs->fs_args,
            &(mntdp->mntd_flags), mntdp->mntd_canon_dev, mntdp->mntd_canon_dir);
	if (rv != 0)
		return -1;

	/*
	 * Switch the default process to the native syscalls.
	 * The bad thing about this is that it will fail in unobvious
	 * way if programs using this go multithreaded, but worry about
	 * that if it happens.
	 */
	rump_i_know_what_i_am_doing_with_sysents = 1;
	rump_pub_lwproc_sysent_usenative();

	if (rump_sys_mkdir(MOUNT_DIRECTORY, 0777) == -1 && errno != EEXIST)
		err(-1, "mkdir");
	strcpy(mntdp->mntd_canon_dir, MOUNT_DIRECTORY);

	rv = fsu_load_fs(fs->fs_name);

	if (rv == 0)
		rv = rump_sys_mount(fs->fs_name, mntdp->mntd_canon_dir,
			mntdp->mntd_flags, fs->fs_args, fs->fs_args_size);

	if (rv == 0) {
		/* fork a rump kernel process to chroot() to the mountpoint */
		if ((rv = rump_pub_lwproc_rfork(RUMP_RFCFDG)) != 0) {
			warnx("fork failed!");
			rump_sys_unmount(MOUNT_DIRECTORY, 0);
		} else {
			atexit(fsu_unmount);
			rump_sys_chroot(MOUNT_DIRECTORY);
		}
	}
#ifdef WITH_SMBFS
        if (strcmp(fs->fs_name, MOUNT_SMBFS) == 0) {
                extern struct smb_ctx sctx;

                smb_ctx_done(&sctx);
        }
#endif
        if (rv != 0 && verbose) {
		warn(NULL);
		fprintf(stderr, "%s is not a valid %s image\n",
		    mntdp->mntd_fsdevice, fs->fs_name);
	}

        return rv;
}

void
fsu_unmount(void)
{

	/*
	 * Release the emulated process.  This:
	 *   1) free up the mountpoint vnode (chroot is gone)
	 *   2) gives us a native process context so we can umount()
	 */
	rump_pub_lwproc_releaselwp();
	if (rump_sys_unmount(MOUNT_DIRECTORY, 0) != 0)
		warnx("unmount failed, image may be dirty!");
}

const char *
fsu_mount_usage(void)
{

#ifdef WITH_SYSPUFFS
	return "[-o mnt_args] [-s specopts] [-t fstype] [-p puffs_exec] [-f] fsdevice";
#else
	return "[-o mnt_args] [-s specopts] [-t fstype] [-f] fsdevice";
#endif
}

static int
fsu_load_fs(const char *fsname)
{
	char fname[MAXPATHLEN + 1];
	void *handle;
	const struct modinfo *const *mi_start, *const *mi_end;
	int error;

	snprintf(fname, sizeof(fname) - 1, "librumpfs_%s.so", fsname);
	handle = dlopen(fname, RTLD_LAZY|RTLD_GLOBAL);
	if (handle == NULL)
		return -1;

	mi_start = dlsym(handle, "__start_link_set_modules");
	mi_end = dlsym(handle, "__stop_link_set_modules");
	if (mi_start && mi_end) {
		error = rump_pub_module_init(mi_start,
		    (size_t)(mi_end-mi_start));
		if (error)
			goto errclose;
		return 0;
	}
	error = EINVAL;

 errclose:
	dlclose(handle);
	errno = error;
	return -1;
}
