/* $Id: DynLoadLibSolaris.h $ */
/** @file
 * Dynamically loaded libraries for Solaris hosts, Internal header.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_SRC_src_server_solaris_DynLoadLibSolaris_h
#define MAIN_INCLUDED_SRC_src_server_solaris_DynLoadLibSolaris_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#define LIB_DLPI "libdlpi.so.1"
#ifdef RT_OS_SOLARIS_10
#include <sys/dlpi.h>
#else
#include <libdlpi.h>
#endif

typedef boolean_t dlpi_walkfunc_t(const char*, void *);

extern int  (*g_pfnLibDlpiWalk)(dlpi_walkfunc_t *, void *, uint_t);
extern int  (*g_pfnLibDlpiOpen)(const char *, dlpi_handle_t *, uint_t);
extern void (*g_pfnLibDlpiClose)(dlpi_handle_t);

extern bool VPoxSolarisLibDlpiFound(void);

#endif /* !MAIN_INCLUDED_SRC_src_server_solaris_DynLoadLibSolaris_h */

