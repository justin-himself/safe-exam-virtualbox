/* $Id: UIToolBar.h $ */
/** @file
 * VPox Qt GUI - UIToolBar class declaration.
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

#ifndef FEQT_INCLUDED_SRC_widgets_UIToolBar_h
#define FEQT_INCLUDED_SRC_widgets_UIToolBar_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QToolBar>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QMainWindow;
class QResizeEvent;
class QWidget;
#ifdef VPOX_WS_MAC
class QPaintEvent;
#endif

/** QToolBar extension with few settings presets. */
class SHARED_LIBRARY_STUFF UIToolBar : public QToolBar
{
    Q_OBJECT;

signals:

    /** Notifies listeners about @a newSize. */
    void sigResized(const QSize &newSize);

public:

    /** Constructs tool-bar passing @a pParent to the base-class. */
    UIToolBar(QWidget *pParent = 0);

    /** Defines whether tool-bar should use text-labels. */
    void setUseTextLabels(bool fEnable);

#ifdef VPOX_WS_MAC
    /** Mac OS X: Defines whether native tool-bar should be enabled. */
    void enableMacToolbar();
    /** Mac OS X: Defines whether native tool-bar should be emulated. */
    void emulateMacToolbar();

    /** Mac OS X: Defines whether native tool-bar button should be shown. */
    void setShowToolBarButton(bool fShow);
    /** Mac OS X: Updates native tool-bar layout. */
    void updateLayout();
#endif

protected:

    /** Handles resize @a pEvent. */
    virtual void resizeEvent(QResizeEvent *pEvent) /* override */;

#ifdef VPOX_WS_MAC
    /** Handles paint @a pEvent. */
    virtual void paintEvent(QPaintEvent *pEvent) /* override */;
#endif

private:

    /** Prepares all. */
    void prepare();

    /** Holds the parent main-window isntance. */
    QMainWindow *m_pMainWindow;

#ifdef VPOX_WS_MAC
    /** Holds whether unified tool-bar should be emulated. */
    bool  m_fEmulateUnifiedToolbar;
#endif
};

#endif /* !FEQT_INCLUDED_SRC_widgets_UIToolBar_h */
