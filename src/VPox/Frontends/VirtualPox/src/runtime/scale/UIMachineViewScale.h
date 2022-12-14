/* $Id: UIMachineViewScale.h $ */
/** @file
 * VPox Qt GUI - UIMachineViewScale class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_scale_UIMachineViewScale_h
#define FEQT_INCLUDED_SRC_runtime_scale_UIMachineViewScale_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Local includes */
#include "UIMachineView.h"

class UIMachineViewScale : public UIMachineView
{
    Q_OBJECT;

protected:

    /* Scale machine-view constructor: */
    UIMachineViewScale(  UIMachineWindow *pMachineWindow
                       , ulong uScreenId
#ifdef VPOX_WITH_VIDEOHWACCEL
                       , bool bAccelerate2DVideo
#endif
    );
    /* Scale machine-view destructor: */
    virtual ~UIMachineViewScale() {}

private slots:

    /* Slot to perform guest resize: */
    void sltPerformGuestScale();

private:

    /* Event handlers: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /** Applies machine-view scale-factor. */
    void applyMachineViewScaleFactor();

    /** Resends guest size-hint. */
    void resendSizeHint();

    /* Private helpers: */
    QSize sizeHint() const;
    QRect workingArea() const;
    QSize calculateMaxGuestSize() const;
    void updateSliders();

    /* Friend classes: */
    friend class UIMachineView;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_scale_UIMachineViewScale_h */

