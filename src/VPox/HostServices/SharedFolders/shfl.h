/** @file
 * Shared Folders: Main header - Common data and function prototypes definitions.
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
 */

#ifndef VPOX_INCLUDED_SRC_SharedFolders_shfl_h
#define VPOX_INCLUDED_SRC_SharedFolders_shfl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VPox/err.h>
#include <VPox/hgcmsvc.h>
#include <VPox/shflsvc.h>

#include <VPox/log.h>

/** Shared Folders client flags.
 * @{
 */
/** Client has queried mappings at least once and, therefore, the service can
 * process its other requests too. */
#define SHFL_CF_MAPPINGS_QUERIED (0x00000001)
/** Mappings have been changed since last query. */
#define SHFL_CF_MAPPINGS_CHANGED (0x00000002)
/** Client uses UTF8 encoding, if not set then unicode 16 bit (UCS2) is used. */
#define SHFL_CF_UTF8             (0x00000004)
/** Client both supports and wants to use symlinks. */
#define SHFL_CF_SYMLINKS         (0x00000008)
/** The call to SHFL_FN_WAIT_FOR_MAPPINGS_CHANGES will return immediately
 *  because of a SHFL_FN_CANCEL_MAPPINGS_CHANGES_WAITS call. */
#define SHFL_CF_CANCEL_NEXT_WAIT (0x00000010)
/** @} */

/**
 * @note This structure is dumped directly into the saved state, so care must be
 *       taken when extending it!
 */
typedef struct SHFLCLIENTDATA
{
    /** Client flags */
    uint32_t fu32Flags;
    /** Path delimiter. */
    RTUTF16  PathDelimiter;
    /** The error style, SHFLERRORSTYLE. */
    uint8_t  enmErrorStyle;
    /** Set if the client has mapping usage counts.
     * This is for helping with saved state. */
    uint8_t  fHasMappingCounts;
    /** Mapping counts for each root ID so we can unmap the folders when the
     *  session disconnects or the VM resets. */
    uint16_t acMappings[SHFL_MAX_MAPPINGS];
} SHFLCLIENTDATA;
/** Pointer to a SHFLCLIENTDATA structure. */
typedef SHFLCLIENTDATA *PSHFLCLIENTDATA;


/** @def SHFL_CLIENT_NEED_WINDOWS_ERROR_STYLE_ADJUST_ON_POSIX
 * Whether to make windows error style adjustments on a posix host.
 * This always returns false on windows hosts. */
#ifdef RT_OS_WINDOWS
# define SHFL_CLIENT_NEED_WINDOWS_ERROR_STYLE_ADJUST_ON_POSIX(a_pClient) (false)
#else
# define SHFL_CLIENT_NEED_WINDOWS_ERROR_STYLE_ADJUST_ON_POSIX(a_pClient) ((a_pClient)->enmErrorStyle == kShflErrorStyle_Windows)
#endif

#endif /* !VPOX_INCLUDED_SRC_SharedFolders_shfl_h */

