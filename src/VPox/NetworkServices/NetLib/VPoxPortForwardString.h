/* $Id: VPoxPortForwardString.h $ */
/** @file
 * VPoxPortForwardString
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VPOX_INCLUDED_SRC_NetLib_VPoxPortForwardString_h
#define VPOX_INCLUDED_SRC_NetLib_VPoxPortForwardString_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/net.h>
#include <VPox/intnet.h>

RT_C_DECLS_BEGIN

#define PF_NAMELEN 64
/*
 * TBD: Here is shared implementation of parsing port-forward string
 * of format:
 *      name:[ipv4 or ipv6 address]:host-port:[ipv4 or ipv6 guest addr]:guest port
 *
 * This code supposed to be used in NetService and Frontend and perhaps in corresponding
 * services.
 *
 * Note: ports are in host format.
 */

typedef struct PORTFORWARDRULE
{
    char       szPfrName[PF_NAMELEN];
    /* true if ipv6 and false otherwise */
    int        fPfrIPv6;
    /* IPPROTO_{UDP,TCP} */
    int        iPfrProto;
    char       szPfrHostAddr[INET6_ADDRSTRLEN];
    uint16_t   u16PfrHostPort;
    char       szPfrGuestAddr[INET6_ADDRSTRLEN];
    uint16_t   u16PfrGuestPort;
} PORTFORWARDRULE, *PPORTFORWARDRULE;

int netPfStrToPf(const char *pszStrPortForward, bool fIPv6, PPORTFORWARDRULE pPfr);

RT_C_DECLS_END

#endif /* !VPOX_INCLUDED_SRC_NetLib_VPoxPortForwardString_h */

