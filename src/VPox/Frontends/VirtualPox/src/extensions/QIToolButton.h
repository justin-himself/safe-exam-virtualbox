/* $Id: QIToolButton.h $ */
/** @file
 * VPox Qt GUI - Qt extensions: QIToolButton class declaration.
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_extensions_QIToolButton_h
#define FEQT_INCLUDED_SRC_extensions_QIToolButton_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QToolButton>

/* GUI includes: */
#include "UILibraryDefs.h"

/** QToolButton subclass with extended functionality. */
class SHARED_LIBRARY_STUFF QIToolButton : public QToolButton
{
    Q_OBJECT;

public:

    /** Constructs tool-button passing @a pParent to the base-class. */
    QIToolButton(QWidget *pParent = 0);

    /** Sets whether the auto-raise status @a fEnabled. */
    virtual void setAutoRaise(bool fEnabled);

    /** Removes the tool-button border. */
    void removeBorder();
};

#endif /* !FEQT_INCLUDED_SRC_extensions_QIToolButton_h */
