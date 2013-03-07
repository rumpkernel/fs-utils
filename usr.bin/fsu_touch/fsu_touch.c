/*	$NetBSD: fsu_touch.c,v 1.3 2009/11/06 11:47:42 stacktic Exp $	*/
/* from */
/*	NetBSD: touch.c,v 1.26 2006/03/18 11:15:00 dsl Exp	*/

/*
 * Copyright (c) 1993
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
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#if HAVE_NBCOMPAT_H
#include <nbcompat.h>
#endif

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#ifdef __NetBSD__
#include <tzfile.h>
#endif
#include <unistd.h>

#include <rump/rump_syscalls.h>

#include <fsu_mount.h>

#ifndef TM_YEAR_BASE
#define TM_YEAR_BASE (1900)
#endif

#define FSU_TOUCH_ACCESS_TIME (0x01)
#define FSU_TOUCH_NO_CREAT (FSU_TOUCH_ACCESS_TIME<<1)
#define FSU_TOUCH_CHANGE_LINK (FSU_TOUCH_NO_CREAT<<1)
#define FSU_TOUCH_CHANGE_MOD_TIME (FSU_TOUCH_CHANGE_LINK<<1)
#define FSU_TOUCH_USE_FILE_TIME (FSU_TOUCH_CHANGE_MOD_TIME<<1)
#define FSU_TOUCH_USE_USER_TIME (FSU_TOUCH_USE_FILE_TIME<<1)



static int	fsu_touch(const char *, char *, char *, int);
static int	fsu_touch_parse_arg(int *, char ***, char **, char **);
static void	stime_arg1(char *, struct timeval *);
static void	stime_file(char *, struct timeval *);
static void	usage(void);

int
main(int argc, char *argv[])
{
	int cur_arg, flags, rv;
	char *rtime, *ttime;

	ttime = rtime = NULL;

	if (fsu_mount(&argc, &argv) == 0)
		errx(-1, NULL);

	flags = fsu_touch_parse_arg(&argc, &argv, &rtime, &ttime);
	if (flags == -1 || argc < 1) {
		usage();
		return -1;
	}

	for (rv = 0, cur_arg = 0; cur_arg < argc; ++cur_arg)
		rv |= fsu_touch(argv[cur_arg], rtime, ttime, flags);

	return rv;
}


static int
fsu_touch(const char *filename, char *rtime, char *ttime,
	  int flags)
{
	struct stat sb;
	struct timeval tv[2];
	int rv, timeset;
	int (*utimes_fun)(const char *,
				 const struct timeval *);
	int (*stat_fun)(const char *, struct stat *);

	if (gettimeofday(&tv[0], NULL)) {
		warn("gettimeofday");
		return -1;
	}

	timeset = 0;
	if (flags & FSU_TOUCH_USE_FILE_TIME) {
		stime_file(rtime, tv);
		timeset = 1;
	} else if (flags & FSU_TOUCH_USE_USER_TIME) {
		stime_arg1(ttime, tv);
		timeset = 1;
	} else {
		tv[1] = tv[0];
	}

	if (flags & FSU_TOUCH_CHANGE_LINK) {
		utimes_fun = rump_sys_lutimes;
		stat_fun = rump_sys_lstat;
	} else {
		utimes_fun = rump_sys_utimes;
		stat_fun = rump_sys_stat;
	}

	rv = stat_fun(filename, &sb);
	if (rv == -1) {
		if (!(flags & FSU_TOUCH_NO_CREAT)) {

			rv = rump_sys_open(filename, DEFFILEMODE);
			if (rv == -1) {
				warn("%s", filename);
				return -1;
			}

			if (!timeset)
				return 0;
		} else {
			return 0;
		}
	}

	if (!(flags & FSU_TOUCH_ACCESS_TIME)) {
#ifdef __linux__
                tv[0].tv_sec = sb.st_atime;
                tv[0].tv_usec = 0;
#else
		TIMESPEC_TO_TIMEVAL(&tv[0], &sb.st_atimespec);
#endif
        }
	if (!(flags & FSU_TOUCH_CHANGE_MOD_TIME)) {
#ifdef __linux__
                tv[1].tv_sec = sb.st_mtime;
                tv[1].tv_usec = 0;
#else
		TIMESPEC_TO_TIMEVAL(&tv[1], &sb.st_mtimespec);
#endif
        }

	rv = utimes_fun(filename, tv);
	if (rv == -1) {
		warn("%s", filename);
		return -1;
	}

	return 0;
}

static int
fsu_touch_parse_arg(int *argc, char ***argv, char **rparam, char **tparam)
{
	int ch, flags;

	flags = 0;
	while ((ch = getopt(*argc, *argv, "achmr:t:")) != -1) {
		switch(ch) {
		case 'a':
			flags |= FSU_TOUCH_ACCESS_TIME;
			break;
		case 'c':
			flags |= FSU_TOUCH_NO_CREAT;
			break;
		case 'h':
			flags |= FSU_TOUCH_CHANGE_LINK | FSU_TOUCH_NO_CREAT;
			break;
		case 'm':
			flags |= FSU_TOUCH_CHANGE_MOD_TIME;
			break;
		case 'r':
			flags |= FSU_TOUCH_USE_FILE_TIME;
			*rparam = optarg;
			break;
		case 't':
			flags |= FSU_TOUCH_USE_USER_TIME;
			*tparam = optarg;
			break;
		case '?':
		default:
			return -1;
		}
	}
	*argc -= optind;
	*argv += optind;

	if ((flags & (FSU_TOUCH_ACCESS_TIME | FSU_TOUCH_CHANGE_MOD_TIME)) == 0)
		flags |= FSU_TOUCH_ACCESS_TIME | FSU_TOUCH_CHANGE_MOD_TIME;

	return flags;
}

#define	ATOI2(s)	((s) += 2, ((s)[-2] - '0') * 10 + ((s)[-1] - '0'))

void
stime_arg1(arg, tvp)
	char *arg;
	struct timeval *tvp;
{
	struct tm *t;
	time_t tmptime;
	int yearset;
	char *p;
					/* Start with the current time. */
	tmptime = tvp[0].tv_sec;
	if ((t = localtime(&tmptime)) == NULL)
		err(1, "localtime");
					/* [[CC]YY]MMDDhhmm[.SS] */
	if ((p = strchr(arg, '.')) == NULL)
		t->tm_sec = 0;		/* Seconds defaults to 0. */
	else {
		if (strlen(p + 1) != 2)
			goto terr;
		*p++ = '\0';
		t->tm_sec = ATOI2(p);
	}

	yearset = 0;
	switch (strlen(arg)) {
	case 12:			/* CCYYMMDDhhmm */
		t->tm_year = ATOI2(arg) * 100 - TM_YEAR_BASE;
		yearset = 1;
		/* FALLTHROUGH */
	case 10:			/* YYMMDDhhmm */
		if (yearset) {
			t->tm_year += ATOI2(arg);
		} else {
			yearset = ATOI2(arg);
			if (yearset < 69)
				t->tm_year = yearset + 2000 - TM_YEAR_BASE;
			else
				t->tm_year = yearset + 1900 - TM_YEAR_BASE;
		}
		/* FALLTHROUGH */
	case 8:				/* MMDDhhmm */
		t->tm_mon = ATOI2(arg);
		--t->tm_mon;		/* Convert from 01-12 to 00-11 */
		/* FALLTHROUGH */
	case 6:
		t->tm_mday = ATOI2(arg);
		/* FALLTHROUGH */
	case 4:
		t->tm_hour = ATOI2(arg);
		/* FALLTHROUGH */
	case 2:
		t->tm_min = ATOI2(arg);
		break;
	default:
		goto terr;
	}

	t->tm_isdst = -1;		/* Figure out DST. */
	tvp[0].tv_sec = tvp[1].tv_sec = mktime(t);
	if (tvp[0].tv_sec == -1) {
terr:		errx(1,
	"out of range or illegal time specification: [[CC]YY]MMDDhhmm[.SS]");
	}
	tvp[0].tv_usec = tvp[1].tv_usec = 0;
}

void
stime_file(char *fname, struct timeval *tvp)
{
	struct stat sb;

	if (rump_sys_stat(fname, &sb))
		warn("%s", fname);
#ifdef __linux__
        tvp[0].tv_sec = sb.st_atime;
        tvp[0].tv_usec = 0;
        tvp[1].tv_sec = sb.st_mtime;
        tvp[1].tv_usec = 0;
#else
	TIMESPEC_TO_TIMEVAL(&tvp[0], &sb.st_atimespec);
	TIMESPEC_TO_TIMEVAL(&tvp[1], &sb.st_mtimespec);
#endif
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s %s [-achm] [-r file] [-t time] file ...\n",
		getprogname(), fsu_mount_usage());

	exit(EXIT_FAILURE);
}
