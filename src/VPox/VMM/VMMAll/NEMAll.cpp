/* $Id: NEMAll.cpp $ */
/** @file
 * NEM - Native execution manager, R0 and R3 context code.
 */

/*
 * Copyright (C) 2018-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_NEM
#include <VPox/vmm/nem.h>
#include "NEMInternal.h"
#include <VPox/vmm/vmcc.h>
#include <VPox/err.h>


/**
 * Checks if this VM is in NEM mode and is long-mode capable.
 *
 * Use VMR3IsLongModeAllowed() instead of this, when possible.
 *
 * @returns true if long mode is allowed, false otherwise.
 * @param   pVM         The cross context VM structure.
 * @sa      VMR3IsLongModeAllowed, HMIsLongModeAllowed
 */
VMM_INT_DECL(bool) NEMHCIsLongModeAllowed(PVMCC pVM)
{
    return pVM->nem.s.fAllow64BitGuests && VM_IS_NEM_ENABLED(pVM);
}


/**
 * Physical access handler registration notification.
 *
 * @param   pVM         The cross context VM structure.
 * @param   enmKind     The kind of access handler.
 * @param   GCPhys      Start of the access handling range.
 * @param   cb          Length of the access handling range.
 *
 * @note    Called while holding down the PGM lock.
 */
VMM_INT_DECL(void) NEMHCNotifyHandlerPhysicalRegister(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb)
{
#ifdef VPOX_WITH_NATIVE_NEM
    if (VM_IS_NEM_ENABLED(pVM))
        nemHCNativeNotifyHandlerPhysicalRegister(pVM, enmKind, GCPhys, cb);
#else
    RT_NOREF(pVM, enmKind, GCPhys, cb);
#endif
}


VMM_INT_DECL(void) NEMHCNotifyHandlerPhysicalModify(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld,
                                                    RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fRestoreAsRAM)
{
#ifdef VPOX_WITH_NATIVE_NEM
    if (VM_IS_NEM_ENABLED(pVM))
        nemHCNativeNotifyHandlerPhysicalModify(pVM, enmKind, GCPhysOld, GCPhysNew, cb, fRestoreAsRAM);
#else
    RT_NOREF(pVM, enmKind, GCPhysOld, GCPhysNew, cb, fRestoreAsRAM);
#endif
}


VMM_INT_DECL(int)  NEMHCNotifyPhysPageAllocated(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys, uint32_t fPageProt,
                                                PGMPAGETYPE enmType, uint8_t *pu2State)
{
    Assert(VM_IS_NEM_ENABLED(pVM));
#ifdef VPOX_WITH_NATIVE_NEM
    return nemHCNativeNotifyPhysPageAllocated(pVM, GCPhys, HCPhys, fPageProt, enmType, pu2State);
#else
    RT_NOREF(pVM, GCPhys, HCPhys, fPageProt, enmType, pu2State);
    return VINF_SUCCESS;
#endif
}


#ifndef VPOX_WITH_NATIVE_NEM
VMM_INT_DECL(int) NEMImportStateOnDemand(PVMCPUCC pVCpu, uint64_t fWhat)
{
    RT_NOREF(pVCpu, fWhat);
    return VERR_NEM_IPE_9;
}
#endif


#ifndef VPOX_WITH_NATIVE_NEM
VMM_INT_DECL(int) NEMHCQueryCpuTick(PVMCPUCC pVCpu, uint64_t *pcTicks, uint32_t *puAux)
{
    RT_NOREF(pVCpu, pcTicks, puAux);
    AssertFailed();
    return VERR_NEM_IPE_9;
}
#endif


#ifndef VPOX_WITH_NATIVE_NEM
VMM_INT_DECL(int) NEMHCResumeCpuTickOnAll(PVMCC pVM, PVMCPUCC pVCpu, uint64_t uPausedTscValue)
{
    RT_NOREF(pVM, pVCpu, uPausedTscValue);
    AssertFailed();
    return VERR_NEM_IPE_9;
}
#endif

