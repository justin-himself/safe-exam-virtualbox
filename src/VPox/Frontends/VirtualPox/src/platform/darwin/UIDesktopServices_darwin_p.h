/* $Id: UIDesktopServices_darwin_p.h $ */
/** @file
 * VPox Qt GUI - Qt GUI - Utility Classes and Functions specific to darwin..
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

#ifndef FEQT_INCLUDED_SRC_platform_darwin_UIDesktopServices_darwin_p_h
#define FEQT_INCLUDED_SRC_platform_darwin_UIDesktopServices_darwin_p_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VPox/VPoxCocoa.h>
#include <iprt/cdefs.h> /* for RT_C_DECLS_BEGIN/RT_C_DECLS_END & stuff */

ADD_COCOA_NATIVE_REF(NSString);

RT_C_DECLS_BEGIN

bool darwinCreateMachineShortcut(NativeNSStringRef pstrSrcFile, NativeNSStringRef pstrDstPath, NativeNSStringRef pstrName, NativeNSStringRef pstrUuid);
bool darwinOpenInFileManager(NativeNSStringRef pstrFile);

RT_C_DECLS_END

#endif /* !FEQT_INCLUDED_SRC_platform_darwin_UIDesktopServices_darwin_p_h */

