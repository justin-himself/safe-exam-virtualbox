/* $Id: UIChooserDefs.h $ */
/** @file
 * VPox Qt GUI - UIChooserDefs class declaration.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_manager_chooser_UIChooserDefs_h
#define FEQT_INCLUDED_SRC_manager_chooser_UIChooserDefs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QGraphicsItem>

/* Other VPox includes: */
#include <iprt/cdefs.h>


/** UIChooserItem types. */
enum UIChooserItemType
{
    UIChooserItemType_Any     = QGraphicsItem::UserType,
    UIChooserItemType_Group,
    UIChooserItemType_Global,
    UIChooserItemType_Machine
};


/** UIChooserItem search flags. */
enum UIChooserItemSearchFlag
{
    UIChooserItemSearchFlag_Machine   = RT_BIT(0),
    UIChooserItemSearchFlag_Global    = RT_BIT(1),
    UIChooserItemSearchFlag_Group     = RT_BIT(2),
    UIChooserItemSearchFlag_ExactName = RT_BIT(3)
};


/** UIChooserItem drag token types. */
enum UIChooserItemDragToken
{
    UIChooserItemDragToken_Off,
    UIChooserItemDragToken_Up,
    UIChooserItemDragToken_Down
};

/** UIChooserItemMachine enumeration flags. */
enum UIChooserItemMachineEnumerationFlag
{
    UIChooserItemMachineEnumerationFlag_Unique       = RT_BIT(0),
    UIChooserItemMachineEnumerationFlag_Inaccessible = RT_BIT(1)
};


#endif /* !FEQT_INCLUDED_SRC_manager_chooser_UIChooserDefs_h */
