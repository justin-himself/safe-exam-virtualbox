/** @file
 * Safe way to include iphlpapi.h.
 */

/*
 * Copyright (C) 2016-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualPox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef IPRT_INCLUDED_win_iphlpapi_h
#define IPRT_INCLUDED_win_iphlpapi_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#pragma warning(push)
#pragma warning(disable:4668) /* 'NDIS_SUPPORT_NDIS6' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' */

#include <iphlpapi.h>

#pragma warning(pop)

#endif /* !IPRT_INCLUDED_win_iphlpapi_h */

