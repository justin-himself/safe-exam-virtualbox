/* $Id: UIChooserHandlerMouse.h $ */
/** @file
 * VPox Qt GUI - UIChooserHandlerMouse class declaration.
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

#ifndef FEQT_INCLUDED_SRC_manager_chooser_UIChooserHandlerMouse_h
#define FEQT_INCLUDED_SRC_manager_chooser_UIChooserHandlerMouse_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QObject>

/* Forward declarations: */
class UIChooserModel;
class QGraphicsSceneMouseEvent;
class UIChooserItem;

/* Mouse event type: */
enum UIMouseEventType
{
    UIMouseEventType_Press,
    UIMouseEventType_Release,
    UIMouseEventType_DoubleClick
};

/* Mouse handler for graphics selector: */
class UIChooserHandlerMouse : public QObject
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIChooserHandlerMouse(UIChooserModel *pParent);

    /* API: Model mouse-event handler delegate: */
    bool handle(QGraphicsSceneMouseEvent *pEvent, UIMouseEventType type) const;

private:

    /* API: Model wrapper: */
    UIChooserModel* model() const;

    /* Helpers: Model mouse-event handler delegates: */
    bool handleMousePress(QGraphicsSceneMouseEvent *pEvent) const;
    bool handleMouseRelease(QGraphicsSceneMouseEvent *pEvent) const;
    bool handleMouseDoubleClick(QGraphicsSceneMouseEvent *pEvent) const;

    /* Variables: */
    UIChooserModel *m_pModel;
};

#endif /* !FEQT_INCLUDED_SRC_manager_chooser_UIChooserHandlerMouse_h */

