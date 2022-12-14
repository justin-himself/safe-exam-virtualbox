/** @file
 * Drag and Drop definitions - Common header for host service and guest clients.
 */

/*
 * Copyright (C) 2018-2020 Oracle Corporation
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

#ifndef VPOX_INCLUDED_GuestHost_DragAndDropDefs_h
#define VPOX_INCLUDED_GuestHost_DragAndDropDefs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

/*
 * The mode of operations.
 */
#define VPOX_DRAG_AND_DROP_MODE_OFF           0
#define VPOX_DRAG_AND_DROP_MODE_HOST_TO_GUEST 1
#define VPOX_DRAG_AND_DROP_MODE_GUEST_TO_HOST 2
#define VPOX_DRAG_AND_DROP_MODE_BIDIRECTIONAL 3

#define VPOX_DND_ACTION_IGNORE     UINT32_C(0)
#define VPOX_DND_ACTION_COPY       RT_BIT_32(0)
#define VPOX_DND_ACTION_MOVE       RT_BIT_32(1)
#define VPOX_DND_ACTION_LINK       RT_BIT_32(2)

/** A single DnD action. */
typedef uint32_t VPOXDNDACTION;
/** A list of (OR'ed) DnD actions. */
typedef uint32_t VPOXDNDACTIONLIST;

#define hasDnDCopyAction(a)   ((a) & VPOX_DND_ACTION_COPY)
#define hasDnDMoveAction(a)   ((a) & VPOX_DND_ACTION_MOVE)
#define hasDnDLinkAction(a)   ((a) & VPOX_DND_ACTION_LINK)

#define isDnDIgnoreAction(a)  ((a) == VPOX_DND_ACTION_IGNORE)
#define isDnDCopyAction(a)    ((a) == VPOX_DND_ACTION_COPY)
#define isDnDMoveAction(a)    ((a) == VPOX_DND_ACTION_MOVE)
#define isDnDLinkAction(a)    ((a) == VPOX_DND_ACTION_LINK)

/** @def VPOX_DND_FORMATS_DEFAULT
 * Default drag'n drop formats.
 * Note: If you add new entries here, make sure you test those
 *       with all supported guest OSes!
 */
#define VPOX_DND_FORMATS_DEFAULT                                                                \
    "text/uri-list",                                                                            \
    /* Text. */                                                                                 \
    "text/html",                                                                                \
    "text/plain;charset=utf-8",                                                                 \
    "text/plain;charset=utf-16",                                                                \
    "text/plain",                                                                               \
    "text/richtext",                                                                            \
    "UTF8_STRING",                                                                              \
    "TEXT",                                                                                     \
    "STRING",                                                                                   \
    /* OpenOffice formats. */                                                                   \
    /* See: https://wiki.openoffice.org/wiki/Documentation/DevGuide/OfficeDev/Common_Application_Features#OpenOffice.org_Clipboard_Data_Formats */ \
    "application/x-openoffice-embed-source-xml;windows_formatname=\"Star Embed Source (XML)\"", \
    "application/x-openoffice;windows_formatname=\"Bitmap\""

#endif /* !VPOX_INCLUDED_GuestHost_DragAndDropDefs_h */

