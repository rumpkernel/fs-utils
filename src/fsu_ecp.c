/* $NetBSD: fsu_ecp.c,v 1.11 2009/11/17 15:27:39 stacktic Exp $ */

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

#include "fs-utils.h"
#include <sys/queue.h>
#include <sys/stat.h>
#ifdef __NetBSD__
#include <sys/syslimits.h>
#elif !defined(PATH_MAX)
#define PATH_MAX (1024)
#endif

#if HAVE_NBCOMPAT_H
#include <nbcompat.h>
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rump_syscalls.h>

#include <fsu_utils.h>
#include <fsu_mount.h>

#include "fsu_flist.h"

#define FSU_ECP_NO_COPY_LINK (0x01)
#define FSU_ECP_RECURSIVE (FSU_ECP_NO_COPY_LINK<<1)
#define FSU_ECP_VERBOSE (FSU_ECP_RECURSIVE<<1)
#define FSU_ECP_GET (FSU_ECP_VERBOSE<<1)
#define FSU_ECP_PUT (FSU_ECP_GET<<1)
#define FSU_ECP_DELETE (FSU_ECP_PUT<<1)



#define BUFSIZE (8192)

static int copy_dir(const char *, const char *, int);
static int copy_dir_rec(const char *, char *, int);
static int copy_fifo(const char *, const char *, int);
static int copy_file(const char *, const char *, int);
static int copy_filein(const char *, const char *);
static int copy_fileout(const char *, const char *);
static int copy_link(const char *, const char *, int);
static int copy_special(const char *, const char *, int);
static int copy_to_dir(const char *, struct stat *,
		       const char *, int);
static int copy_to_file(const char *, struct stat *,
			const char *, int);
static int fsu_ecp(const char *, const char *, int);
static int fsu_ecp_parse_arg(int *, char ***);
static void usage(void);

struct hardlink_s {
	char *hl_from;
	char **hl_to;
	int hl_nlink;
	LIST_ENTRY(hardlink_s) next;
};

int
main(int argc, char *argv[])
{
	size_t len;
	int cur_arg, flags, rv;

	setprogname(argv[0]);
	if (fsu_mount(&argc, &argv, MOUNT_READWRITE) != 0)
		usage();

	flags = fsu_ecp_parse_arg(&argc, &argv);
	if (flags == -1 || argc < 2) {
		usage();
		return -1;
	}

	for (rv = 0, cur_arg = 0; cur_arg < argc-1; ++cur_arg) {
		len = strlen(argv[cur_arg]);
		while (len != 1 && argv[cur_arg][len - 1] == '/')
			argv[cur_arg][--len] = '\0';
		rv |= fsu_ecp(argv[cur_arg], argv[argc-1], flags);
	}

	return rv;
}

static int
fsu_ecp_parse_arg(int *argc, char ***argv)
{
	int flags, rv;
	const char *progname;

	flags = 0;
	progname = getprogname();

	if (strcmp(progname, "get") == 0 || strcmp(progname, "fsu_get") == 0)
		flags |= FSU_ECP_GET;
	else if (strcmp(progname, "put") == 0 ||
		 strcmp(progname, "fsu_put") == 0)
		flags |= FSU_ECP_PUT;
	else if (strcmp(progname, "fsu_emv") == 0)
		flags |= FSU_ECP_DELETE;

	while ((rv = getopt(*argc, *argv, "dgLpRv")) != -1) {
		switch (rv) {
		case 'd':
			flags |= FSU_ECP_DELETE;
			break;
		case 'g':
			flags |= FSU_ECP_GET;
			flags &= ~FSU_ECP_PUT;
			break;
		case 'L':
			flags |= FSU_ECP_NO_COPY_LINK;
			break;
		case 'p':
			flags |= FSU_ECP_PUT;
			flags &= ~FSU_ECP_GET;
			break;
		case 'R':
			flags |= FSU_ECP_RECURSIVE;
			break;
		case 'v':
			flags |= FSU_ECP_VERBOSE;
			break;
		case '?':
		default:
			return -1;
		}
	}
	*argc -= optind;
	*argv += optind;

	if ((flags & (FSU_ECP_GET | FSU_ECP_PUT)) == 0) {
		warnx("-g or -p should be specified");
		return -1;
	}

	return flags;
}

static int
fsu_ecp(const char *from, const char *to, int flags)
{
	struct stat from_stat, to_stat;
	int rv;

	if (flags & FSU_ECP_PUT)
		rv = lstat(from, &from_stat);
	else
		rv = rump_sys_lstat(from, &from_stat);
	if (rv == -1) {
		warn("%s", from);
		return -1;
	}

	if (S_ISDIR(from_stat.st_mode)) {
		if (!(flags & FSU_ECP_RECURSIVE)) {
			fprintf(stderr, "%s: is a directory\n", from);
			return -1;
		}
		return copy_dir(from, to, flags);
	}

	if (flags & FSU_ECP_GET)
		rv = stat(to, &to_stat);
	else
		rv = rump_sys_stat(to, &to_stat);
	if (rv == 0 && S_ISDIR(to_stat.st_mode))
		return copy_to_dir(from, &from_stat, to, flags);

	return copy_to_file(from, &from_stat, to, flags);
}

static int
copy_dir(const char *from, const char *to, int flags)
{
	const char *filename;
	int rv;
	struct stat to_stat;
	char to_p[PATH_MAX + 1];
	size_t tlen, flen;

	tlen = strlen(to);
	rv = strlcpy(to_p, to, PATH_MAX + 1);
	if (rv != (int)tlen) {
		warn("%s", to);
		return -1;
	}
	if (to_p[tlen - 1] != '/')
	    	to_p[tlen++] = '/';
	to_p[tlen] = 0;

	if (flags & FSU_ECP_GET)
		rv = lstat(to, &to_stat);
	else
		rv = rump_sys_lstat(to, &to_stat);

	if (rv == 0 && !S_ISDIR(to_stat.st_mode)) {
		warnx("%s: not a directory", to);
		return -1;
	}

	if (rv == -1) {
		if (flags & FSU_ECP_GET) {
			rv = mkdir(to, 0777);
			if (rv == -1)
				return -1;
			rv = lstat(to, &to_stat);
		} else {
			rv = rump_sys_mkdir(to, 0777);
			if (rv == -1)
				return -1;
			rv = rump_sys_lstat(to, &to_stat);
		}

	}
	if (rv == -1) {
		warn("%s", to);
		return -1;
	}
	filename = strrchr(from, '/');
	if (filename == NULL)
		filename = from;
	else
		++filename;

	flen = strlen(filename);

	rv = strlcat(to_p, filename, PATH_MAX + 1);
	if (rv != (int)(flen + tlen)) {
		warn("%s/%s", to_p, filename);
		return -1;
	}
	to_p[rv] = 0;
	return copy_dir_rec(from, to_p, flags);
}

#ifndef offsetof
#define offsetof(a, b) ((uintptr_t)(&((a *)NULL)->b))
#endif
#define NEXT_OFFSET (offsetof(FSU_FENT, next))
#define LIST_PREV(elm, field) ((elm)->field.le_prev == NULL ?  NULL :\
		(void*)((uintptr_t)((elm)->field.le_prev) - NEXT_OFFSET))

static int
fsu_remove_directory_tree(fsu_flist *list, FSU_FENT *last, int flags)
{
	FSU_FENT *cur, *root;
	int (*rmdirf)(const char *pathname);
	int (*unlinkf)(const char *pathname);
	int rv;

	rv = 0;
	rmdirf = flags & FSU_ECP_GET ? rump_sys_rmdir : rmdir;
	unlinkf = flags & FSU_ECP_GET ? rump_sys_unlink : unlink;
	root = LIST_FIRST(list);

	for (cur = last;
	    cur != NULL;
	    cur = LIST_PREV(cur, next)) {

		if (S_ISDIR(cur->sb.st_mode))
			rv = rmdirf(cur->path);
		else
			rv  = unlinkf(cur->path);
		if (rv == -1) {
			warn("%s", cur->path);
			break;
		}

		if (flags & FSU_ECP_VERBOSE)
			printf("Removing %s\n", cur->path);
		if (cur == root)
			break;
	}
	return rv;
}

static int
copy_dir_rec(const char *from_p, char *to_p, int flags)
{
	FSU_FENT *root, *cur, *cur2, *nextelt;
	fsu_flist *flist;
	struct stat sb;
	size_t len;
	int flist_options, res, rv, off, hl_supported, curlink, do_delete;
	struct hardlink_s *new;
	char hlfrom[PATH_MAX + 1], hlto[PATH_MAX + 1];

	LIST_HEAD(, hardlink_s) hl_l = LIST_HEAD_INITIALIZER(hl_l);

	do_delete = flags & FSU_ECP_DELETE;
	flags &= ~FSU_ECP_DELETE;
	res = 0;
	hl_supported = 1;

	if (flags & FSU_ECP_NO_COPY_LINK)
		flist_options = 0;
	else
		flist_options = FSU_FLIST_STATLINK;

	if (flags & FSU_ECP_RECURSIVE)
		flist_options |= FSU_FLIST_RECURSIVE;

	if (flags & FSU_ECP_PUT)
		flist_options |= FSU_FLIST_REALFS;

	len = strlen(to_p) - 1;

	flist = fsu_flist_build(from_p, flist_options);
	if (flist == NULL)
		return -1;
	root = LIST_FIRST(flist);

	off = root->pathlen;
	LIST_FOREACH(cur, flist, next) {
		if (S_ISDIR(cur->sb.st_mode) || cur->sb.st_nlink == 1)
			continue;

		new = malloc(sizeof(struct hardlink_s));
		if (new == NULL) {
			warn("malloc");
			return -1;
		}
		new->hl_from = cur->path;
		new->hl_nlink = cur->sb.st_nlink - 1;
		new->hl_to = malloc(new->hl_nlink * sizeof(char *));
		if (new->hl_to == NULL) {
			warn("malloc");
			free(new);
			return -1;
		}
		memset(new->hl_to, 0, new->hl_nlink * sizeof(char *));

		for (curlink = 0, cur2 = cur; LIST_NEXT(cur2, next) != NULL;) {
			nextelt = LIST_NEXT(cur2, next);
			if (S_ISDIR(nextelt->sb.st_mode) ||
			    nextelt->sb.st_nlink == 1) {
				cur2 = nextelt;
				continue;
			}
			if (flags & FSU_ECP_GET)
				rump_sys_lstat(nextelt->path, &sb);
			else if (flags & FSU_ECP_PUT)
				lstat(nextelt->path, &sb);

			if (S_ISLNK(sb.st_mode)) {
				cur2 = nextelt;
				continue;
			}

			if (cur->sb.st_ino == nextelt->sb.st_ino &&
			    cur->sb.st_dev == nextelt->sb.st_dev) {
				new->hl_to[curlink] = nextelt->path;
				if (new->hl_to[curlink] == NULL)
					warn("%s", nextelt->path);
				else
					curlink++;
				LIST_REMOVE(nextelt, next);
				free(nextelt);
				if ((unsigned)(curlink + 1) == cur->sb.st_nlink)
					break;
			} else
				cur2 = nextelt;
		}
		LIST_INSERT_HEAD(&hl_l, new, next);
	}

	if (flags & FSU_ECP_GET)
		rv = mkdir(to_p, root->sb.st_mode);
	else
		rv = rump_sys_mkdir(to_p, root->sb.st_mode);
	if (rv == -1) {
		if (errno == EEXIST) {
			if (flags & FSU_ECP_GET)
				rv = stat(to_p, &sb);
			else
				rv = rump_sys_stat(to_p, &sb);
			if (!S_ISDIR(sb.st_mode)) {
				errno = ENOTDIR;
				warn("%s", to_p);
				goto out;
			}
		} else {
			warn("%s", to_p);
			goto out;
		}
	}

	if (!(flags & FSU_ECP_GET)) {
		rv = rump_sys_chown(to_p, root->sb.st_uid, root->sb.st_gid);
		if (rv == -1) {
			warn("chown %s", to_p);
		}
	}

	LIST_FOREACH(cur, flist, next) {
		if (cur == root)
			continue;
		rv = strlcat(to_p, cur->path + off, PATH_MAX+1);
		if (rv != (int)(len + cur->pathlen - off + 1)) {
			warn("%s%s", to_p, cur->path + off);
			res = -1;
			break;
		}

		if (S_ISDIR(cur->sb.st_mode)) {
			if (flags & FSU_ECP_GET) {
				rv = mkdir(to_p, cur->sb.st_mode);
				if (rv == -1) {
					warn("%s", to_p);
					res = -1;
					break;
				}
			} else {
				rv = rump_sys_mkdir(to_p, cur->sb.st_mode);
				if (rv == -1) {
					warn("%s", to_p);
					res = -1;
					break;
				}

				rv = rump_sys_chown(to_p, cur->sb.st_uid,
						cur->sb.st_gid);
				if (rv == -1) {
					warn("chown %s", to_p);
				}
			}
		} else {
			res |= copy_to_file(cur->path, &(cur->sb), to_p,
					  flags);
			if (errno == ENOSPC) {
				warn(NULL);
				goto out;
			}
		}
		to_p[len + 1] = '\0';
		cur2 = cur;
	}

	if (do_delete && res == 0)
		fsu_remove_directory_tree(flist, cur2, flags);

	memcpy(hlfrom, to_p, len + 1);
	memcpy(hlto, to_p, len + 1);

	while (!LIST_EMPTY(&hl_l)) {
		new = LIST_FIRST(&hl_l);
		LIST_REMOVE(new, next);

		hlfrom[len + 1] = '\0';
		strlcat(hlfrom, new->hl_from + off, PATH_MAX+1);

		for (curlink = 0;
		     curlink < new->hl_nlink && new->hl_to[curlink] != NULL;
		     ++curlink) {

			hlto[len + 1] = '\0';
			strlcat(hlto, new->hl_to[curlink] + off, PATH_MAX+1);

			if (hl_supported) {
				if (flags & FSU_ECP_GET)
					rv = link(hlfrom, hlto);
				else
					rv = rump_sys_link(hlfrom, hlto);
				if (rv != 0 && errno == EOPNOTSUPP) {
					hl_supported = 0;
					if (flags & FSU_ECP_GET)
						copy_fileout(hlfrom, hlto);
					else
						copy_filein(hlfrom, hlto);
				}
			} else {
				if (flags & FSU_ECP_GET)
					copy_fileout(hlfrom, hlto);
				else
					copy_filein(hlfrom, hlto);
			}
			free(new->hl_to[curlink]);
		}
		free(new->hl_to);
		free(new);
	}

out:
	fsu_flist_free(flist);

	return res;
}

static int
copy_to_dir(const char *from, struct stat *frstat,
	    const char *to, int flags)
{
	char path[PATH_MAX+1];
	const char *filename;
	int len;

	filename = strrchr(from, '/');
	if (filename == NULL)
		filename = from;
	else
		++filename;

	len = snprintf(path, PATH_MAX, "%s/%s", to, filename);
	path[len] = '\0';

	return copy_to_file(from, frstat, path, flags);
}

static int
copy_to_file(const char *from, struct stat *frstat,
	     const char *to, int flags)
{
	int rv;

	switch ((frstat->st_mode & S_IFMT)) {
	case S_IFIFO:
		rv = copy_fifo(from, to, flags);
		break;
	case S_IFLNK:
		if (!(flags & FSU_ECP_NO_COPY_LINK))
			rv = copy_link(from, to, flags);
		else
			rv = copy_file(from, to, flags);
		break;
	case S_IFCHR: /* FALLTHROUGH */
	case S_IFBLK:
		rv = copy_special(from, to, flags);
		break;
	case S_IFREG:
		rv = copy_file(from, to, flags);
		break;
	default:
		return -1;
		/* NOTREACHED */
	}

	if (rv != 0)
		return rv;

	if (flags & FSU_ECP_DELETE) {
		if (flags & FSU_ECP_GET)
			rv = rump_sys_unlink(from);
		else
			rv = unlink(from);
	}

	if (flags & FSU_ECP_GET)
		return rv;

	if (!(flags & FSU_ECP_NO_COPY_LINK))
		rv = rump_sys_lchown(to, frstat->st_uid, frstat->st_gid);
	else
		rv = rump_sys_chown(to, frstat->st_uid, frstat->st_gid);
	if (rv == -1) {
		warn("chown %s", to);
	}

	return 0;
}

static int
copy_file(const char *from, const char *to, int flags)
{
	uint8_t buf[BUFSIZE];
	ssize_t rd, wr;
	off_t off;
	int fdfrom, fdto, rv;
	struct stat from_stat;

	if (flags & FSU_ECP_VERBOSE)
		printf("%s -> %s\n", from, to);

	if (flags & FSU_ECP_PUT)
		rv = stat(from, &from_stat);
	else
		rv = rump_sys_stat(from, &from_stat);
	if (rv == -1) {
		warn("%s", from);
		return -1;
	}

	if (flags & FSU_ECP_GET) {
		fdto = open(to, O_WRONLY|O_CREAT|O_EXCL, 0777);
		fdfrom = rump_sys_open(from, O_RDONLY);
	} else if (flags & FSU_ECP_PUT) {
		fdfrom = open(from, O_RDONLY);
		fdto = rump_sys_open(to, from_stat.st_mode|O_CREAT, 0777);
	} else {
		fdfrom = rump_sys_open(from, O_RDONLY);
		fdto = rump_sys_open(to, from_stat.st_mode|O_CREAT, 0777);
	}
	if (rv == -1) {
		warn("%s", to);
		return -1;
	}

	off = 0;
	do {
		if (flags & FSU_ECP_PUT)
			rd = read(fdfrom, buf, sizeof(buf));
		else
			rd = rump_sys_read(fdfrom, buf, sizeof(buf));
		if (rd == -1) {
			warn("%s", from);
			rv = -1;
			goto out;
		}
		if (flags & FSU_ECP_GET)
			wr = write(fdto, buf, rd);
		else
			wr = rump_sys_write(fdto, buf, rd);
		if (wr == -1 || wr != rd) {
			warn("%s", to);
			rv = -1;
			goto out;
		}
		off += rd;
	} while (rd > 0);

	rv = 0;
out:

	if (flags & FSU_ECP_GET) {
		close(fdto);
		rump_sys_close(fdfrom);
	} else if (flags & FSU_ECP_PUT) {
		close(fdfrom);
		rump_sys_close(fdto);
	} else {
		rump_sys_close(fdfrom);
		rump_sys_close(fdto);
	}

	return rv;
}

static int
copy_fifo(const char *from, const char *to, int flags)
{
	int rv;
	struct stat file_stat;

	if (flags & FSU_ECP_VERBOSE)
		printf("%s -> %s\n", from, to);

	if (flags & FSU_ECP_GET)
		rv = lstat(to, &file_stat);
	else
		rv = rump_sys_lstat(to, &file_stat);
	if (rv != -1) {
		if (flags & FSU_ECP_GET)
			rv = remove(to);
		else
			rv = rump_sys_unlink(to);
		if (rv == -1) {
			warn("%s", to);
			return -1;
		}
	}

	if (flags & FSU_ECP_PUT)
		rv = lstat(from, &file_stat);
	else
		rv = rump_sys_lstat(from, &file_stat);
	if (rv == -1) {
		warn("%s", from);
		return -1;
	}

	if (flags & FSU_ECP_GET)
		rv = mkfifo(to, file_stat.st_mode);
	else
		rv = rump_sys_mkfifo(to, file_stat.st_mode);
	if (rv == -1) {
		warn("%s", to);
		return -1;
	}

	return 0;
}

static int
copy_special(const char *from, const char *to, int flags)
{
	int rv;
	struct stat file_stat;

	if (flags & FSU_ECP_VERBOSE)
		printf("%s -> %s\n", from, to);

	if (flags & FSU_ECP_GET)
		rv = lstat(to, &file_stat);
	else
		rv = rump_sys_lstat(to, &file_stat);
	if (rv != -1) {
		rv = rump_sys_unlink(to);
		if (rv == -1) {
			warn("%s", to);
			return -1;
		}
	}

	if (flags & FSU_ECP_PUT)
		rv = lstat(from, &file_stat);
	else
		rv = rump_sys_lstat(from, &file_stat);
	if (rv == -1) {
		warn("%s", from);
		return -1;
	}

	if (flags & FSU_ECP_GET)
		rv = mknod(to, file_stat.st_mode, file_stat.st_rdev);
	else
		rv = rump_sys_mknod(to, file_stat.st_mode, file_stat.st_rdev);
	if (rv == -1) {
		warn("%s", to);
		return -1;
	}

	return 0;
}

static int
copy_link(const char *from, const char *to, int flags)
{
	char target[PATH_MAX+1];
	int rv;
	struct stat file_stat;

	if (flags & FSU_ECP_PUT)
		rv = readlink(from, target, PATH_MAX);
	else
		rv = rump_sys_readlink(from, target, PATH_MAX);
	if (rv == -1) {
		warn("%s", from);
		return -1;
	}
	target[rv] = '\0';

	if (flags & FSU_ECP_VERBOSE)
		printf("%s -> %s : %s\n", from, to, target);

	if (flags & FSU_ECP_GET)
		rv = lstat(to, &file_stat);
	else
		rv = rump_sys_lstat(to, &file_stat);
	if (rv != -1) {
		if (flags & FSU_ECP_GET)
			rv = remove(to);
		else
			rv = rump_sys_unlink(to);
		if (rv == -1) {
			warn("%s", to);
			return -1;
		}
	}

	if (flags & FSU_ECP_GET)
		rv = symlink(target, to);
	else
		rv = rump_sys_symlink(target, to);
	if (rv == -1) {
		warn("%s", target);
		return -1;
	}
	return 0;
}

static int
copy_filein(const char *from, const char *to)
{
	uint8_t buf[BUFSIZE];
	ssize_t rd, wr;
	off_t off;
	int rv, fd1, fd2;
	struct stat from_stat;

	rv = rump_sys_stat(from, &from_stat);
	if (rv == -1) {
		warn("%s", from);
		return -1;
	}

	fd1 = rump_sys_open(from, O_RDONLY);
	if (fd1 == -1) {
		warn("%s", to);
		return -1;
	}
	fd2 = rump_sys_open(to, O_WRONLY|O_CREAT, 0777);

	off = 0;
	do {
		rd = rump_sys_read(fd1, buf, sizeof(buf));
		if (rd == -1) {
			warn("%s", from);
			return -1;
		}
		wr = rump_sys_write(fd2, buf, rd);
		if (wr == -1 || wr != rd) {
			warn("%s", to);
			return -1;
		}
		off += rd;
	} while (rd > 0);
	rump_sys_close(fd1);
	rump_sys_close(fd2);

	return 0;
}

static int
copy_fileout(const char *from, const char *to)
{
	uint8_t buf[BUFSIZE];
	ssize_t rd, wr;
	int rv, fdfrom, fdto;
	struct stat from_stat;

	rv = stat(from, &from_stat);
	if (rv == -1) {
		warn("%s", from);
		return -1;
	}

	fdto = open(to, O_CREAT | O_WRONLY, from_stat.st_mode);
	if (fdto == -1) {
		warn("%s", to);
		return -1;
	}
	fdfrom = open(from, O_RDONLY, from_stat.st_mode);
	if (fdfrom == -1) {
		warn("%s", from);
		close(fdto);
		return -1;
	}

	do {
		rd = read(fdfrom, buf, sizeof(buf));
		if (rd == -1) {
			warn("%s", from);
			rv = -1;
			goto out;
		}
		wr = write(fdto, buf, rd);
		if (wr == -1 || wr != rd) {
			warn("%s", to);
			rv = -1;
			goto out;
		}
	} while (rd > 0);


out:
	close(fdfrom);
	close(fdto);
	return rv;
}

static void
usage(void)
{

	fprintf(stderr,	"usage: %s %s [-gLpRv] src target\n"
		"usage: %s %s [-gLpRv] src... directory\n",
		getprogname(), fsu_mount_usage(),
		getprogname(), fsu_mount_usage());

	exit(EXIT_FAILURE);
}
