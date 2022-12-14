/* $Id: UIErrorPane.h $ */
/** @file
 * VPox Qt GUI - UIErrorPane class declaration.
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

#ifndef FEQT_INCLUDED_SRC_manager_UIErrorPane_h
#define FEQT_INCLUDED_SRC_manager_UIErrorPane_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QAction;
class QString;
class QTextBrowser;
class QToolButton;


/** QWidget subclass representing error pane reflecting information
  * about currently chosen inaccessible VM and allowing to operate over it. */
class UIErrorPane : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs error pane passing @a pParent to the base-class.
      * @param  pRefreshAction  Brings the refresh action reference. */
    UIErrorPane(QAction *pRefreshAction = 0, QWidget *pParent = 0);

    /** Defines error @a strDetails. */
    void setErrorDetails(const QString &strDetails);

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private:

    /** Prepares all. */
    void prepare();

    /** Holds the Refresh action refrence. */
    QAction     *m_pActionRefresh;
    /** Holds the VM refresh button instance. */
    QToolButton *m_pButtonRefresh;

    /** Holds the text-browser instance. */
    QTextBrowser *m_pBrowserDetails;
};

#endif /* !FEQT_INCLUDED_SRC_manager_UIErrorPane_h */
