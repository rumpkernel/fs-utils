/*	$NetBSD: smb_conn.h,v 1.20 2010/12/17 13:05:29 pooka Exp $	*/

/*
 * Copyright (c) 2000-2001 Boris Popov
 * All rights reserved.
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
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * FreeBSD: src/sys/netsmb/smb_conn.h,v 1.8 2002/09/16 10:50:38 bp Exp
 */

#ifndef _NETSMB_SMB_CONN_H_
#define _NETSMB_SMB_CONN_H_

/*
 * Two levels of connection hierarchy
 */
#define	SMBL_SM		0
#define SMBL_VC		1
#define SMBL_SHARE	2
#define SMBL_NUM	3
#define SMBL_NONE	(-1)

#define	SMB_CS_NONE	0x0000
#define	SMB_CS_UPPER	0x0001		/* convert passed string to upper case */
#define	SMB_CS_LOWER	0x0002		/* convert passed string to lower case */

/*
 * Common object flags
 */
#define SMBO_GONE		0x1000000

/*
 * access modes
 */
#define	SMBM_READ		0400	/* read conn attrs.(like list shares) */
#define	SMBM_WRITE		0200	/* modify conn attrs */
#define	SMBM_EXEC		0100	/* can send SMB requests */
#define	SMBM_READGRP		0040
#define	SMBM_WRITEGRP		0020
#define	SMBM_EXECGRP		0010
#define	SMBM_READOTH		0004
#define	SMBM_WRITEOTH		0002
#define	SMBM_EXECOTH		0001
#define	SMBM_MASK		0777
#define	SMBM_EXACT		010000	/* check for specified mode exactly */
#define	SMBM_ALL		(SMBM_READ | SMBM_WRITE | SMBM_EXEC)
#define	SMBM_DEFAULT		(SMBM_READ | SMBM_WRITE | SMBM_EXEC)
#define	SMBM_ANY_OWNER		((uid_t)-1)
#define	SMBM_ANY_GROUP		((gid_t)-1)

/*
 * VC flags
 */
#define SMBV_PERMANENT		0x0002
#define SMBV_LONGNAMES		0x0004	/* connection is configured to use long names */
#define	SMBV_ENCRYPT		0x0008	/* server asked for encrypted password */
#define	SMBV_WIN95		0x0010	/* used to apply bugfixes for this OS */
#define	SMBV_PRIVATE		0x0020	/* connection can be used only by creator */
#define	SMBV_RECONNECTING	0x0040	/* conn is in the process of reconnection */
#define SMBV_SINGLESHARE	0x0080	/* only one share connecting should be allowed */
#define SMBV_CREATE		0x0100	/* lookup for create operation */
/*#define SMBV_FAILED		0x0200*/	/* last reconnect attempt has failed */
#define SMBV_UNICODE		0x0400	/* connection is configured to use Unicode */


/*
 * smb_share flags
 */
#define SMBS_PERMANENT		0x0001
#define SMBS_RECONNECTING	0x0002
#define SMBS_CONNECTED		0x0004

/*
 * share types
 */
#define	SMB_ST_DISK		0x0	/* A: */
#define	SMB_ST_PRINTER		0x1	/* LPT: */
#define	SMB_ST_PIPE		0x2	/* IPC */
#define	SMB_ST_COMM		0x3	/* COMM */
#define	SMB_ST_ANY		0x4
#define	SMB_ST_MAX		0x4
#define SMB_ST_NONE		0xff	/* not a part of protocol */

/*
 * Negotiated protocol parameters
 */
struct smb_sopt {
	int		sv_proto;
	int16_t		sv_tz;		/* offset in min relative to UTC */
	u_int32_t	sv_maxtx;	/* maximum transmit buf size */
	u_int16_t	sv_sm;		/* security mode */
	u_int16_t	sv_maxmux;	/* max number of outstanding rq's */
	u_int16_t 	sv_maxvcs;	/* max number of VCs */
	u_int16_t	sv_rawmode;
	u_int32_t	sv_maxraw;	/* maximum raw-buffer size */
	u_int32_t	sv_skey;	/* session key */
	u_int32_t	sv_caps;	/* capabilities SMB_CAP_ */
};

/*
 * network IO daemon states
 */
enum smbiod_state {
	SMBIOD_ST_NOTCONN,	/* no connect request was made */
	SMBIOD_ST_RECONNECT,	/* a [re]connect attempt is in progress */
	SMBIOD_ST_TRANACTIVE,	/* transport level is up */
	SMBIOD_ST_VCACTIVE,	/* session established */
	SMBIOD_ST_DEAD		/* connection broken, transport is down */
};


/*
 * Info structures
 */
#define	SMB_INFO_NONE		0
#define	SMB_INFO_VC		2
#define	SMB_INFO_SHARE		3

struct smb_vc_info {
	int		itype;
	int		usecount;
	uid_t		uid;		/* user id of connection */
	gid_t		gid;		/* group of connection */
	mode_t		mode;		/* access mode */
	int		flags;
	enum smbiod_state iodstate;
	struct smb_sopt	sopt;
	char		srvname[SMB_MAXSRVNAMELEN];
	char		vcname[128];
};

struct smb_share_info {
	int		itype;
	int		usecount;
	u_short		tid;		/* TID */
	int		type;		/* share type */
	uid_t		uid;		/* user id of connection */
	gid_t		gid;		/* group of connection */
	mode_t		mode;		/* access mode */
	int		flags;
	char		sname[128];
};
#endif /* !_NETSMB_SMB_CONN_H_ */
