/* $Id: tftp.h $ */
/** @file
 * NAT - TFTP server (declarations/defines).
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* tftp defines */

#ifndef _SLIRP_TFTP_H_
#define _SLIRP_TFTP_H_

#define TFTP_SESSIONS_MAX 3

#define TFTP_SERVER     69

#define TFTP_RRQ    1
#define TFTP_WRQ    2
#define TFTP_DATA   3
#define TFTP_ACK    4
#define TFTP_ERROR  5
#define TFTP_OACK   6

/* error codes */
#define TFTP_EUNDEF     0 /* Not defined, see error message (if any). */
#define TFTP_ENOENT     1 /* File not found. */
#define TFTP_EACCESS    2 /* Access violation. */
#define TFTP_EFBIG      3 /* Disk full or allocation exceeded. */
#define TFTP_ENOSYS     4 /* Illegal TFTP operation. */
#define TFTP_ESRCH      5 /* Unknown transfer ID. */
#define TFTP_EEXIST     6 /* File already exists. */
#define TFTP_EUSER      7 /* No such user. */
/* RFC 2347 */
#define TFTP_EONAK      8 /* Option refused. */


#define TFTP_FILENAME_MAX 512


int  slirpTftpInput(PNATState pData, struct mbuf *m);
int  slirpTftpInit(PNATState pData);
void slirpTftpTerm(PNATState pData);
#endif
