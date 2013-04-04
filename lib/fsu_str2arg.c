/* $NetBSD: fsu_str2arg.c,v 1.3 2009/11/05 14:02:43 stacktic Exp $ */

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

#ifdef __linux__
#define _XOPEN_SOURCE 500
#define _BSD_SOURCE
#endif

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <fsu_utils.h>

/*
 * get argc and argv from a string
 */
char
*fsu_str2arg(char *str, int *argc, char **argv, int vsize)
{
	char *p, *lp, delim;
	bool inquote;

	inquote = false;
	delim = '\0';

	*argc = 1;
	argv[0] = str;

	for (lp = p = str; lp[0] != '\0' && *argc < vsize; ++lp) {
		if (lp[0] == '\\')
			*p++ = *++lp;
		else if (inquote && lp[0] == delim)
			inquote = false;
		else if (lp[0] == '"' || lp[0] == '\'') {
			delim = lp[0];
			inquote = true;
		} else if (isspace((int)lp[0]) && !inquote) {
			*p++ ='\0';

			while (isspace((int)lp[1]))
				++lp;

			if (lp[1] != '\0')
				argv[(*argc)++] = p;
		} else
			*p++ = *lp;
	}
	argv[*argc] = NULL;
	*p = '\0';

	if (inquote)
		fprintf(stderr, "Unbalanced quote, processing as if there was "
			"one at the end of the command line\n");
	return argv[0];
}

int
fsu_str2argc(char *str)
{
	int argc;
	char *p, *lp, delim;
	bool inquote;

	inquote = false;
	delim = '\0';

	argc = 1;

	for (lp = p = str; lp[0] != '\0'; ++lp) {
		if (inquote && lp[0] == delim)
			inquote = false;
		else if (lp[0] == '"' || lp[0] == '\'') {
			delim = lp[0];
			inquote = true;
		} else if (isspace((int)lp[0]) && !inquote) {
			++p;

			while (isspace((int)lp[1]))
				++lp;

			if (lp[1] != '\0')
				++argc;
		} else
			p++;
	}
	return argc;
}
