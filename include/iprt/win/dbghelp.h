/** @file
 * Safe way to include Dbghelp.h.
 */

/*
 * Copyright (C) 2020 Oracle Corporation
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

#ifndef IPRT_INCLUDED_win_dbghelp_h
#define IPRT_INCLUDED_win_dbghelp_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifdef _MSC_VER
/*
 * Unfortunately, the Windows.h file in SDK 7.1 is not clean wrt warning C4091 with VCC140+:
 *      Dbghelp.h(1540): warning C4091: 'typedef ': ignored on left of '<unnamed-enum-hdBase>' when no variable is declared
 *      Dbghelp.h(3056): warning C4091: 'typedef ': ignored on left of '<unnamed-enum-sfImage>' when no variable is declared
 */
# pragma warning(push)
# if _MSC_VER >= 1900 /*RT_MSC_VER_VC140*/
#  pragma warning(disable:4091)
# endif
#endif

#include <Dbghelp.h>

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#endif /* !IPRT_INCLUDED_win_dbghelp_h */

