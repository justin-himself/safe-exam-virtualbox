/** @file
 * CPUM - Disassembler.
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

#ifndef VPOX_INCLUDED_vmm_cpumdis_h
#define VPOX_INCLUDED_vmm_cpumdis_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VPox/vmm/cpum.h>
#include <iprt/x86.h>
#include <VPox/dis.h>


RT_C_DECLS_BEGIN
/** @addtogroup grp_cpum
 * @{
 */

#ifdef IN_RING3
VMMR3DECL(int) CPUMR3DisasmInstrCPU(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, RTGCPTR GCPtrPC, PDISCPUSTATE pCpu, const char *pszPrefix);
#endif

/** @} */
RT_C_DECLS_END


#endif /* !VPOX_INCLUDED_vmm_cpumdis_h */

