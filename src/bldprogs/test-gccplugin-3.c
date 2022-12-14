/* $Id: test-gccplugin-3.c $ */
/** @file
 * Compiler plugin testcase \#3.
 */

/*
 * Copyright (C) 2010-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Only valid stuff in this one. */
#if defined(__i386__) || defined(_M_IX86) || defined(__X86__)
# define RTCALL __attribute__((__cdecl__,__regparm__(0)))
#else
# define RTCALL
#endif
typedef struct HELPERS
{
    void (RTCALL * pfnPrintf)(struct HELPERS *pHlp, const char *pszFormat, ...)
        __attribute__((__iprt_format__(2, 3)));
} HELPERS;

extern void foo(struct HELPERS *pHlp);


void foo(struct HELPERS *pHlp)
{
    pHlp->pfnPrintf(pHlp, "%36 %#x %#x", "string", 42, 42); /// @todo missing 's', need to detect this.
}

