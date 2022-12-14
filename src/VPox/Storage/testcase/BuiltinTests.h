/** @file
 *
 * tstVDIo testing utility - builtin tests.
 */

/*
 * Copyright (C) 2014-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VPOX_INCLUDED_SRC_testcase_BuiltinTests_h
#define VPOX_INCLUDED_SRC_testcase_BuiltinTests_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/**
 * Builtin Tests (in generated BuiltinTests.cpp)
 */
typedef struct TSTVDIOTESTENTRY
{
    /** Test name. */
    const char             *pszName;
    /** Pointer to the raw bytes. */
    const unsigned char    *pch;
    /** Number of bytes. */
    unsigned                cb;
} TSTVDIOTESTENTRY;
/** Pointer to a trust anchor table entry. */
typedef TSTVDIOTESTENTRY const *PCTSTVDIOTESTENTRY;

/** Macro for simplifying generating the trust anchor tables. */
#define TSTVDIOTESTENTRY_GEN(a_szName, a_abTest)      { #a_szName, &a_abTest[0], sizeof(a_abTest) }

/** All tests we know. */
extern TSTVDIOTESTENTRY const       g_aVDIoTests[];
/** Number of entries in g_aVDIoTests. */
extern unsigned const               g_cVDIoTests;

#endif /* !VPOX_INCLUDED_SRC_testcase_BuiltinTests_h */
