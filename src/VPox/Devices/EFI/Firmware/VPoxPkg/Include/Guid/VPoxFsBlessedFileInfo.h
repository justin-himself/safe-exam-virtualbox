/* $Id */
/** @file
 * Provides a GUID and a data structure that can be used with EFI_FILE_PROTOCOL.GetInfo()
 * or EFI_FILE_PROTOCOL.SetInfo() to get or set the system's volume blessed file (for booting).
 */

/*
 * Copyright (C) 2019-2020 Oracle Corporation
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

#ifndef __VPOX_FS_BLESSED_FILE_INFO_H__
#define __VPOX_FS_BLESSED_FILE_INFO_H__

#define VPOX_FS_BLESSED_FILE_ID \
  { \
    0xCC49FEFD, 0x41B7, 0x9823, { 0x98, 0x23, 0x0E, 0x8E, 0xBF, 0x35, 0x67, 0x7D } \
  }

typedef struct {
  ///
  /// The Null-terminated string that is the volume's label.
  ///
  CHAR16  BlessedFile[1];
} VPOX_FS_BLESSED_FILE;

#define SIZE_OF_VPOX_FS_BLESSED_FILE \
        OFFSET_OF (VPOX_FS_BLESSED_FILE, BlessedFile)

extern EFI_GUID gVPoxFsBlessedFileInfoGuid;

#endif
