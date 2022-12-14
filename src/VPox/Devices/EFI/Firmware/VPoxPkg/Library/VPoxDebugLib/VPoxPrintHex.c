/* $Id: VPoxPrintHex.c $ */
/** @file
 * VPoxPrintHex.c - Implementation of the VPoxPrintHex() debug logging routine.
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



/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <Library/BaseLib.h>

#include "VPoxDebugLib.h"
#include "DevEFI.h"
#include "iprt/asm.h"


/**
 * Prints a char.
 * @param   ch              The char to print.
 */
DECLINLINE(void) vpoxPrintHexChar(int ch)
{
    ASMOutU8(EFI_DEBUG_PORT, (uint8_t)ch);
}


/**
 * Print a hex number, up to 64-bit long.
 *
 * @returns Number of chars printed.
 *
 * @param   uValue          The value.
 * @param   cbType          The size of the value type.
 */
size_t VPoxPrintHex(UINT64 uValue, size_t cbType)
{
    static const char s_szHex[17] = "0123456789abcdef";
    switch (cbType)
    {
        case 8:
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 60) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 56) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 52) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 48) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 44) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 40) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 36) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 32) & 0xf]);
        case 4:
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 28) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 24) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 20) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 16) & 0xf]);
        case 2:
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue, 12) & 0xf]);
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue,  8) & 0xf]);
        case 1:
            vpoxPrintHexChar(s_szHex[RShiftU64(uValue,  4) & 0xf]);
            vpoxPrintHexChar(s_szHex[         (uValue    ) & 0xf]);
            break;
    }
    return cbType * 2;
}

