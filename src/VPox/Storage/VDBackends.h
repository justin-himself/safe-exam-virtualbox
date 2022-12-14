/* $Id: VDBackends.h $ */
/** @file
 * VD - builtin backends.
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/

#ifndef VPOX_INCLUDED_SRC_Storage_VDBackends_h
#define VPOX_INCLUDED_SRC_Storage_VDBackends_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VPox/vd-plugin.h>

#include <iprt/cdefs.h>

RT_C_DECLS_BEGIN

extern const VDIMAGEBACKEND g_RawBackend;
extern const VDIMAGEBACKEND g_VmdkBackend;
extern const VDIMAGEBACKEND g_VDIBackend;
extern const VDIMAGEBACKEND g_VhdBackend;
extern const VDIMAGEBACKEND g_ParallelsBackend;
extern const VDIMAGEBACKEND g_DmgBackend;
extern const VDIMAGEBACKEND g_ISCSIBackend;
extern const VDIMAGEBACKEND g_QedBackend;
extern const VDIMAGEBACKEND g_QCowBackend;
extern const VDIMAGEBACKEND g_VhdxBackend;
extern const VDIMAGEBACKEND g_CueBackend;
extern const VDIMAGEBACKEND g_VPoxIsoMakerBackend;

extern const VDCACHEBACKEND g_VciCacheBackend;

RT_C_DECLS_END

#endif /* !VPOX_INCLUDED_SRC_Storage_VDBackends_h */

