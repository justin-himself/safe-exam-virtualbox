/* $Id: VPoxUtils-win.h $ */
/** @file
 * VPox Qt GUI - Declarations of utility classes and functions for handling Windows specific tasks.
 */

/*
 * Copyright (C) 2011-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_platform_win_VPoxUtils_win_h
#define FEQT_INCLUDED_SRC_platform_win_VPoxUtils_win_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QRegion>

/* GUI includes: */
#include "UILibraryDefs.h"

/* External includes: */
#include <iprt/win/windows.h>

/* Namespace for native window sub-system functions: */
namespace NativeWindowSubsystem
{
    /* Returns area covered by visible always-on-top (top-most) windows: */
    SHARED_LIBRARY_STUFF const QRegion areaCoveredByTopMostWindows();
}

#endif /* !FEQT_INCLUDED_SRC_platform_win_VPoxUtils_win_h */
