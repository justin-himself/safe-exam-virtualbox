/* $Id: VPoxDbgBase.cpp $ */
/** @file
 * VPox Debugger GUI - Base classes.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DBGG
#include <iprt/errcore.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <limits.h>
#include "VPoxDbgBase.h"
#include "VPoxDbgGui.h"

#include <QApplication>
#include <QWidgetList>



VPoxDbgBase::VPoxDbgBase(VPoxDbgGui *a_pDbgGui)
    : m_pDbgGui(a_pDbgGui), m_pUVM(NULL), m_hGUIThread(RTThreadNativeSelf())
{
    NOREF(m_pDbgGui); /* shut up warning. */

    /*
     * Register
     */
    m_pUVM = a_pDbgGui->getUvmHandle();
    if (m_pUVM)
    {
        VMR3RetainUVM(m_pUVM);

        int rc = VMR3AtStateRegister(m_pUVM, atStateChange, this);
        AssertRC(rc);
    }
}


VPoxDbgBase::~VPoxDbgBase()
{
    /*
     * If the VM is still around.
     */
    /** @todo need to do some locking here?  */
    PUVM pUVM = ASMAtomicXchgPtrT(&m_pUVM, NULL, PUVM);
    if (pUVM)
    {
        int rc = VMR3AtStateDeregister(pUVM, atStateChange, this);
        AssertRC(rc);

        VMR3ReleaseUVM(pUVM);
    }
}


int
VPoxDbgBase::stamReset(const QString &rPat)
{
    QByteArray Utf8Array = rPat.toUtf8();
    const char *pszPat = !rPat.isEmpty() ? Utf8Array.constData() : NULL;
    PUVM pUVM = m_pUVM;
    if (    pUVM
        &&  VMR3GetStateU(pUVM) < VMSTATE_DESTROYING)
        return STAMR3Reset(pUVM, pszPat);
    return VERR_INVALID_HANDLE;
}


int
VPoxDbgBase::stamEnum(const QString &rPat, PFNSTAMR3ENUM pfnEnum, void *pvUser)
{
    QByteArray Utf8Array = rPat.toUtf8();
    const char *pszPat = !rPat.isEmpty() ? Utf8Array.constData() : NULL;
    PUVM pUVM = m_pUVM;
    if (    pUVM
        &&  VMR3GetStateU(pUVM) < VMSTATE_DESTROYING)
        return STAMR3Enum(pUVM, pszPat, pfnEnum, pvUser);
    return VERR_INVALID_HANDLE;
}


int
VPoxDbgBase::dbgcCreate(PDBGCBACK pBack, unsigned fFlags)
{
    PUVM pUVM = m_pUVM;
    if (    pUVM
        &&  VMR3GetStateU(pUVM) < VMSTATE_DESTROYING)
        return DBGCCreate(pUVM, pBack, fFlags);
    return VERR_INVALID_HANDLE;
}


/*static*/ DECLCALLBACK(void)
VPoxDbgBase::atStateChange(PUVM pUVM, VMSTATE enmState, VMSTATE /*enmOldState*/, void *pvUser)
{
    VPoxDbgBase *pThis = (VPoxDbgBase *)pvUser; NOREF(pUVM);
    switch (enmState)
    {
        case VMSTATE_TERMINATED:
        {
            /** @todo need to do some locking here?  */
            PUVM pUVM2 = ASMAtomicXchgPtrT(&pThis->m_pUVM, NULL, PUVM);
            if (pUVM2)
            {
                Assert(pUVM2 == pUVM);
                pThis->sigTerminated();
                VMR3ReleaseUVM(pUVM2);
            }
            break;
        }

        case VMSTATE_DESTROYING:
            pThis->sigDestroying();
            break;

        default:
            break;
    }
}


void
VPoxDbgBase::sigDestroying()
{
}


void
VPoxDbgBase::sigTerminated()
{
}




//
//
//
//  V B o x D b g B a s e W i n d o w
//  V B o x D b g B a s e W i n d o w
//  V B o x D b g B a s e W i n d o w
//
//
//

unsigned VPoxDbgBaseWindow::m_cxBorder = 0;
unsigned VPoxDbgBaseWindow::m_cyBorder = 0;


VPoxDbgBaseWindow::VPoxDbgBaseWindow(VPoxDbgGui *a_pDbgGui, QWidget *a_pParent, const char *a_pszTitle)
    : QWidget(a_pParent, Qt::Window), VPoxDbgBase(a_pDbgGui), m_pszTitle(a_pszTitle), m_fPolished(false)
    , m_x(INT_MAX), m_y(INT_MAX), m_cx(0), m_cy(0)
{
    /* Set the title, using the parent one as prefix when possible: */
    if (!parent())
    {
        QString strMachineName = a_pDbgGui->getMachineName();
        if (strMachineName.isEmpty())
            setWindowTitle(QString("VPoxDbg - %1").arg(m_pszTitle));
        else
            setWindowTitle(QString("%1 - VPoxDbg - %2").arg(strMachineName).arg(m_pszTitle));
    }
    else
    {
        setWindowTitle(QString("%1 - %2").arg(parentWidget()->windowTitle()).arg(m_pszTitle));

        /* Install an event filter so we can make adjustments when the parent title changes: */
        parent()->installEventFilter(this);
    }
}


VPoxDbgBaseWindow::~VPoxDbgBaseWindow()
{

}


void
VPoxDbgBaseWindow::vShow()
{
    show();
    /** @todo this ain't working right. HELP! */
    setWindowState(windowState() & ~Qt::WindowMinimized);
    //activateWindow();
    //setFocus();
    vPolishSizeAndPos();
}


void
VPoxDbgBaseWindow::vReposition(int a_x, int a_y, unsigned a_cx, unsigned a_cy, bool a_fResize)
{
    if (a_fResize)
    {
        m_cx = a_cx;
        m_cy = a_cy;

        QSize BorderSize = frameSize() - size();
        if (BorderSize == QSize(0,0))
            BorderSize = vGuessBorderSizes();

        resize(a_cx - BorderSize.width(), a_cy - BorderSize.height());
    }

    m_x = a_x;
    m_y = a_y;
    move(a_x, a_y);
}


bool
VPoxDbgBaseWindow::event(QEvent *a_pEvt)
{
    bool fRc = QWidget::event(a_pEvt);
    vPolishSizeAndPos();
    return fRc;
}


bool VPoxDbgBaseWindow::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    /* We're only interested in title changes to the parent so we can amend our own title: */
    if (   pWatched == parent()
        && pEvent->type() == QEvent::WindowTitleChange)
        setWindowTitle(QString("%1 - %2").arg(parentWidget()->windowTitle()).arg(m_pszTitle));

    /* Forward to base-class: */
    return QWidget::eventFilter(pWatched, pEvent);
}


void
VPoxDbgBaseWindow::vPolishSizeAndPos()
{
    /* Ignore if already done or no size set. */
    if (    m_fPolished
        || (m_x == INT_MAX && m_y == INT_MAX))
        return;

    QSize BorderSize = frameSize() - size();
    if (BorderSize != QSize(0,0))
        m_fPolished = true;

    vReposition(m_x, m_y, m_cx, m_cy, m_cx || m_cy);
}


QSize
VPoxDbgBaseWindow::vGuessBorderSizes()
{
#ifdef Q_WS_X11 /* (from the qt gui) */
    /*
     * On X11, there is no way to determine frame geometry (including WM
     * decorations) before the widget is shown for the first time.  Stupidly
     * enumerate other top level widgets to find the thickest frame.
     */
    if (!m_cxBorder && !m_cyBorder) /* (only till we're successful) */
    {
        int cxExtra = 0;
        int cyExtra = 0;

        QWidgetList WidgetList = QApplication::topLevelWidgets();
        for (QListIterator<QWidget *> it(WidgetList); it.hasNext(); )
        {
            QWidget *pCurWidget = it.next();
            if (pCurWidget->isVisible())
            {
                int const cxFrame = pCurWidget->frameGeometry().width()  - pCurWidget->width();
                cxExtra = qMax(cxExtra, cxFrame);
                int const cyFrame = pCurWidget->frameGeometry().height() - pCurWidget->height();
                cyExtra = qMax(cyExtra, cyFrame);
                if (cyExtra && cxExtra)
                    break;
            }
        }

        if (cxExtra || cyExtra)
        {
            m_cxBorder = cxExtra;
            m_cyBorder = cyExtra;
        }
    }
#endif /* X11 */
    return QSize(m_cxBorder, m_cyBorder);
}

