/** $Id: VDDefs.h $ */
/** @file
 *
 * VPox HDD container test utility, common definitions.
 */

/*
 * Copyright (C) 2013-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VPOX_INCLUDED_SRC_testcase_VDDefs_h
#define VPOX_INCLUDED_SRC_testcase_VDDefs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/sg.h>

/**
 * I/O transfer direction.
 */
typedef enum VDIOTXDIR
{
    /** Read. */
    VDIOTXDIR_READ = 0,
    /** Write. */
    VDIOTXDIR_WRITE,
    /** Flush. */
    VDIOTXDIR_FLUSH,
    /** Invalid. */
    VDIOTXDIR_INVALID
} VDIOTXDIR;

#endif /* !VPOX_INCLUDED_SRC_testcase_VDDefs_h */
