/* $Id: DBGPlugIns.h $ */
/** @file
 * DBGPlugIns - Debugger Plug-Ins.
 *
 * This is just a temporary static wrapper for what may eventually
 * become some fancy dynamic stuff.
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

#ifndef DEBUGGER_INCLUDED_SRC_DBGPlugIns_h
#define DEBUGGER_INCLUDED_SRC_DBGPlugIns_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VPox/vmm/dbgf.h>

RT_C_DECLS_BEGIN

extern const DBGFOSREG g_DBGDiggerFreeBsd;
extern const DBGFOSREG g_DBGDiggerDarwin;
extern const DBGFOSREG g_DBGDiggerLinux;
extern const DBGFOSREG g_DBGDiggerOS2;
extern const DBGFOSREG g_DBGDiggerSolaris;
extern const DBGFOSREG g_DBGDiggerWinNt;

RT_C_DECLS_END

#endif /* !DEBUGGER_INCLUDED_SRC_DBGPlugIns_h */

