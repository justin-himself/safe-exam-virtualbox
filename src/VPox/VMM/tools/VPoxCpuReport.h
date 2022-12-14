/* $Id: VPoxCpuReport.h $ */
/** @file
 * VPoxCpuReport internal header file.
 */

/*
 * Copyright (C) 2013-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VMM_INCLUDED_SRC_tools_VPoxCpuReport_h
#define VMM_INCLUDED_SRC_tools_VPoxCpuReport_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VPox/sup.h>

RT_C_DECLS_BEGIN

typedef struct VBCPUREPMSRACCESSORS
{
    /** Wheter MSR prober can read/modify/restore MSRs more or less
     *  atomically, without allowing other code to be executed. */
    bool                    fAtomic;
    /** @copydoc SUPR3MsrProberRead  */
    DECLCALLBACKMEMBER(int, pfnMsrProberRead)(uint32_t uMsr, RTCPUID idCpu, uint64_t *puValue, bool *pfGp);
    /** @copydoc SUPR3MsrProberWrite  */
    DECLCALLBACKMEMBER(int, pfnMsrProberWrite)(uint32_t uMsr, RTCPUID idCpu, uint64_t uValue, bool *pfGp);
    /** @copydoc SUPR3MsrProberModify */
    DECLCALLBACKMEMBER(int, pfnMsrProberModify)(uint32_t uMsr, RTCPUID idCpu, uint64_t fAndMask, uint64_t fOrMask,
                                                PSUPMSRPROBERMODIFYRESULT pResult);
    /** Termination callback, optional. */
    DECLCALLBACKMEMBER(void, pfnTerm)(void);
} VBCPUREPMSRACCESSORS;
typedef VBCPUREPMSRACCESSORS *PVBCPUREPMSRACCESSORS;

extern void vbCpuRepDebug(const char *pszMsg, ...);
extern void vbCpuRepPrintf(const char *pszMsg, ...);
extern int  VbCpuRepMsrProberInitSupDrv(PVBCPUREPMSRACCESSORS pMsrAccessors);
extern int  VbCpuRepMsrProberInitPlatform(PVBCPUREPMSRACCESSORS pMsrAccessors);

RT_C_DECLS_END

#endif /* !VMM_INCLUDED_SRC_tools_VPoxCpuReport_h */

