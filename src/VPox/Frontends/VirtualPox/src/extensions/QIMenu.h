/* $Id: QIMenu.h $ */
/** @file
 * VPox Qt GUI - Qt extensions: QIMenu class declaration.
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

#ifndef FEQT_INCLUDED_SRC_extensions_QIMenu_h
#define FEQT_INCLUDED_SRC_extensions_QIMenu_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMenu>

/* GUI includes: */
#include "UILibraryDefs.h"

/** QMenu extension with advanced functionality.
  * Allows to highlight first menu item for popped up menu. */
class SHARED_LIBRARY_STUFF QIMenu : public QMenu
{
    Q_OBJECT;

public:

    /** Constructs menu passing @a pParent to the base-class. */
    QIMenu(QWidget *pParent = 0);

private slots:

    /** Highlights first menu action for popped up menu. */
    void sltHighlightFirstAction();
};

#endif /* !FEQT_INCLUDED_SRC_extensions_QIMenu_h */
