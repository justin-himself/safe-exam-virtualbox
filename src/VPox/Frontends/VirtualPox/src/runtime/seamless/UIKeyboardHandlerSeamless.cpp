/* $Id: UIKeyboardHandlerSeamless.cpp $ */
/** @file
 * VPox Qt GUI - UIKeyboardHandlerSeamless class implementation.
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

#ifndef VPOX_WS_MAC
/* Qt includes: */
# include <QKeyEvent>
# include <QTimer>
#endif /* !VPOX_WS_MAC */

/* GUI includes: */
#include "UIKeyboardHandlerSeamless.h"
#ifndef VPOX_WS_MAC
# include "UIMachineLogic.h"
# include "UIShortcutPool.h"
#endif /* !VPOX_WS_MAC */


#ifndef VPOX_WS_MAC
/* Namespaces: */
using namespace UIExtraDataDefs;
#endif /* !VPOX_WS_MAC */

UIKeyboardHandlerSeamless::UIKeyboardHandlerSeamless(UIMachineLogic* pMachineLogic)
    : UIKeyboardHandler(pMachineLogic)
{
}

UIKeyboardHandlerSeamless::~UIKeyboardHandlerSeamless()
{
}

#ifndef VPOX_WS_MAC
bool UIKeyboardHandlerSeamless::eventFilter(QObject *pWatchedObject, QEvent *pEvent)
{
    /* Check if pWatchedObject object is view: */
    if (UIMachineView *pWatchedView = isItListenedView(pWatchedObject))
    {
        /* Get corresponding screen index: */
        ulong uScreenId = m_views.key(pWatchedView);
        NOREF(uScreenId);
        /* Handle view events: */
        switch (pEvent->type())
        {
            case QEvent::KeyPress:
            {
                /* Get key-event: */
                QKeyEvent *pKeyEvent = static_cast<QKeyEvent*>(pEvent);
                /* Process Host+Home for menu popup: */
                if (   isHostKeyPressed()
                    && gShortcutPool->shortcut(GUI_Input_MachineShortcuts, QString("PopupMenu")).sequences().contains(QKeySequence(pKeyEvent->key())))
                {
                    /* Post request to show popup-menu: */
                    QTimer::singleShot(0, m_pMachineLogic, SLOT(sltInvokePopupMenu()));
                    /* Filter-out this event: */
                    return true;
                }
                break;
            }
            default:
                break;
        }
    }

    /* Else just propagate to base-class: */
    return UIKeyboardHandler::eventFilter(pWatchedObject, pEvent);
}
#endif /* !VPOX_WS_MAC */

