/** @file
 * IPRT / No-CRT - fenv.h wrapper.
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

#ifndef IPRT_INCLUDED_nocrt_fenv_h
#define IPRT_INCLUDED_nocrt_fenv_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#ifdef RT_ARCH_AMD64
# include <iprt/nocrt/amd64/fenv.h>
#elif defined(RT_ARCH_X86)
# include <iprt/nocrt/x86/fenv.h>
#else
# error "IPRT: no fenv.h available for this platform, or the platform define is missing!"
#endif

#endif /* !IPRT_INCLUDED_nocrt_fenv_h */
