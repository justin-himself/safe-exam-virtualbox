/* $Id: QILabelSeparator.h $ */
/** @file
 * VPox Qt GUI - Qt extensions: QILabelSeparator class declaration.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_extensions_QILabelSeparator_h
#define FEQT_INCLUDED_SRC_extensions_QILabelSeparator_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* GUI inlcudes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QLabel;
class QString;
class QWidget;

/** QWidget extension providing GUI with label-separator. */
class SHARED_LIBRARY_STUFF QILabelSeparator : public QWidget
{
    Q_OBJECT;

public:

    /** Constructs label-separator passing @a pParent and @a fFlags to the base-class. */
    QILabelSeparator(QWidget *pParent = 0, Qt::WindowFlags fFlags = 0);
    /** Constructs label-separator passing @a pParent and @a fFlags to the base-class.
      * @param  strText  Brings the label text. */
    QILabelSeparator(const QString &strText, QWidget *pParent = 0, Qt::WindowFlags fFlags = 0);

    /** Returns the label text. */
    QString text() const;
    /** Defines the label buddy. */
    void setBuddy(QWidget *pBuddy);

public slots:

    /** Clears the label text. */
    void clear();
    /** Defines the label @a strText. */
    void setText(const QString &strText);

protected:

    /** Prepares all. */
    virtual void prepare();

    /** Holds the label instance. */
    QLabel *m_pLabel;
};

#endif /* !FEQT_INCLUDED_SRC_extensions_QILabelSeparator_h */
