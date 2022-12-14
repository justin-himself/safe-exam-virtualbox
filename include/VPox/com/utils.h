/** @file
 * MS COM / XPCOM Abstraction Layer - COM initialization / shutdown.
 */

/*
 * Copyright (C) 2005-2020 Oracle Corporation
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

#ifndef VPOX_INCLUDED_com_utils_h
#define VPOX_INCLUDED_com_utils_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "iprt/types.h"

/** @addtogroup grp_com
 * @{
 */

namespace com
{

/**
 * Returns the VirtualPox user home directory.
 *
 * On failure, this function will return a path that caused a failure (or
 * NULL if the failure is not path-related).
 *
 * On success, this function will try to create the returned directory if it
 * doesn't exist yet. This may also fail with the corresponding status code.
 *
 * If @a aDirLen is smaller than RTPATH_MAX then there is a great chance that
 * this method will return VERR_BUFFER_OVERFLOW.
 *
 * @param   aDir        Buffer to store the directory string in UTF-8 encoding.
 * @param   aDirLen     Length of the supplied buffer including space for the
 *                      terminating null character, in bytes.
 * @param   fCreateDir  Flag whether to create the returned directory on success
 *                      if it doesn't exist.
 * @returns VPox status code.
 */
int GetVPoxUserHomeDirectory(char *aDir, size_t aDirLen, bool fCreateDir = true);

/**
 * Creates a release log file, used both in VPoxSVC and in API clients.
 *
 * @param   pszEntity        Human readable name of the program.
 * @param   pszLogFile       Name of the release log file.
 * @param   fFlags           Logger instance flags.
 * @param   pszGroupSettings Group logging settings.
 * @param   pszEnvVarBase    Base environment variable name for the logger.
 * @param   fDestFlags       Logger destination flags.
 * @param   cMaxEntriesPerGroup Limit for log entries per group. UINT32_MAX for no limit.
 * @param   cHistory         Number of old log files to keep.
 * @param   uHistoryFileTime Maximum amount of time to put in a log file.
 * @param   uHistoryFileSize Maximum size of a log file before rotating.
 * @param   pErrInfo         Where to return extended error information.
 *                           Optional.
 *
 * @returns VPox status code.
 */
int VPoxLogRelCreate(const char *pszEntity, const char *pszLogFile,
                     uint32_t fFlags, const char *pszGroupSettings,
                     const char *pszEnvVarBase, uint32_t fDestFlags,
                     uint32_t cMaxEntriesPerGroup, uint32_t cHistory,
                     uint32_t uHistoryFileTime, uint64_t uHistoryFileSize,
                     PRTERRINFO pErrInfo);

} /* namespace com */

/** @} */
#endif /* !VPOX_INCLUDED_com_utils_h */

