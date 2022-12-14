/* $Id: PDMInline.h $ */
/** @file
 * PDM - Internal header file containing the inlined functions.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VMM_INCLUDED_SRC_include_PDMInline_h
#define VMM_INCLUDED_SRC_include_PDMInline_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/**
 * Calculates the next IRQ tag.
 *
 * @returns IRQ tag.
 * @param   pVM                 The cross context VM structure.
 * @param   idTracer            The ID of the source device.
 */
DECLINLINE(uint32_t) pdmCalcIrqTag(PVM pVM, uint32_t idTracer)
{
    uint32_t uTag = (pVM->pdm.s.uIrqTag + 1) & 0x3ff; /* {0..1023} */
    if (!uTag)
        uTag++;
    pVM->pdm.s.uIrqTag = uTag |= (idTracer << 16);
    return uTag;
}

#endif /* !VMM_INCLUDED_SRC_include_PDMInline_h */

