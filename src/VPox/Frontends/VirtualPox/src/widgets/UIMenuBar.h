/* $Id: UIMenuBar.h $ */
/** @file
 * VPox Qt GUI - UIMenuBar class declaration.
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

#ifndef FEQT_INCLUDED_SRC_widgets_UIMenuBar_h
#define FEQT_INCLUDED_SRC_widgets_UIMenuBar_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMenuBar>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QPaintEvent;
class QWidget;

/** QMenuBar extension
  * which reflects BETA label when necessary. */
class SHARED_LIBRARY_STUFF UIMenuBar: public QMenuBar
{
    Q_OBJECT;

public:

    /** Constructor, passes @a pParent to the QMenuBar constructor. */
    UIMenuBar(QWidget *pParent = 0);

protected:

    /** Paint event handler. */
    virtual void paintEvent(QPaintEvent *pEvent) /* override */;

private:

    /** Reflects whether we should show BETA label or not. */
    bool m_fShowBetaLabel;
};

#endif /* !FEQT_INCLUDED_SRC_widgets_UIMenuBar_h */
