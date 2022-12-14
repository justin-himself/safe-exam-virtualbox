/* $Id: VPoxDbgGui.cpp $ */
/** @file
 * VPox Debugger GUI - The Manager.
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
#define VPOX_COM_NO_ATL
#include <VPox/com/defs.h>
#include <iprt/errcore.h>

#include "VPoxDbgGui.h"
#include <QDesktopWidget>
#include <QApplication>



VPoxDbgGui::VPoxDbgGui() :
    m_pDbgStats(NULL), m_pDbgConsole(NULL), m_pSession(NULL), m_pConsole(NULL),
    m_pMachineDebugger(NULL), m_pMachine(NULL), m_pUVM(NULL),
    m_pParent(NULL), m_pMenu(NULL),
    m_x(0), m_y(0), m_cx(0), m_cy(0), m_xDesktop(0), m_yDesktop(0), m_cxDesktop(0), m_cyDesktop(0)
{

}


int VPoxDbgGui::init(PUVM pUVM)
{
    /*
     * Set the VM handle and update the desktop size.
     */
    m_pUVM = pUVM; /* Note! This eats the incoming reference to the handle! */
    updateDesktopSize();

    return VINF_SUCCESS;
}


int VPoxDbgGui::init(ISession *pSession)
{
    int rc = VERR_GENERAL_FAILURE;

    /*
     * Query the VirtualPox interfaces.
     */
    m_pSession = pSession;
    m_pSession->AddRef();

    HRESULT hrc = m_pSession->COMGETTER(Machine)(&m_pMachine);
    if (SUCCEEDED(hrc))
    {
        hrc = m_pSession->COMGETTER(Console)(&m_pConsole);
        if (SUCCEEDED(hrc))
        {
            hrc = m_pConsole->COMGETTER(Debugger)(&m_pMachineDebugger);
            if (SUCCEEDED(hrc))
            {
                /*
                 * Get the VM handle.
                 */
                LONG64 llVM;
                hrc = m_pMachineDebugger->COMGETTER(VM)(&llVM);
                if (SUCCEEDED(hrc))
                {
                    PUVM pUVM = (PUVM)(intptr_t)llVM;
                    rc = init(pUVM);
                    if (RT_SUCCESS(rc))
                        return rc;

                    VMR3ReleaseUVM(pUVM);
                }

                /* damn, failure! */
                m_pMachineDebugger->Release();
                m_pMachineDebugger = NULL;
            }
            m_pConsole->Release();
            m_pConsole = NULL;
        }
        m_pMachine->Release();
        m_pMachine = NULL;
    }

    return rc;
}


VPoxDbgGui::~VPoxDbgGui()
{
    if (m_pDbgStats)
    {
        delete m_pDbgStats;
        m_pDbgStats = NULL;
    }

    if (m_pDbgConsole)
    {
        delete m_pDbgConsole;
        m_pDbgConsole = NULL;
    }

    if (m_pMachineDebugger)
    {
        m_pMachineDebugger->Release();
        m_pMachineDebugger = NULL;
    }

    if (m_pConsole)
    {
        m_pConsole->Release();
        m_pConsole = NULL;
    }

    if (m_pMachine)
    {
        m_pMachine->Release();
        m_pMachine = NULL;
    }

    if (m_pSession)
    {
        m_pSession->Release();
        m_pSession = NULL;
    }

    if (m_pUVM)
    {
        VMR3ReleaseUVM(m_pUVM);
        m_pUVM = NULL;
    }
}

void
VPoxDbgGui::setParent(QWidget *pParent)
{
    m_pParent = pParent;
}


void
VPoxDbgGui::setMenu(QMenu *pMenu)
{
    m_pMenu = pMenu;
}


int
VPoxDbgGui::showStatistics()
{
    if (!m_pDbgStats)
    {
        m_pDbgStats = new VPoxDbgStats(this, "*", 2, m_pParent);
        connect(m_pDbgStats, SIGNAL(destroyed(QObject *)), this, SLOT(notifyChildDestroyed(QObject *)));
        repositionStatistics();
    }

    m_pDbgStats->vShow();
    return VINF_SUCCESS;
}


void
VPoxDbgGui::repositionStatistics(bool fResize/* = true*/)
{
    /*
     * Move it to the right side of the VPox console,
     * and resize it to cover all the space to the left side of the desktop.
     */
    if (m_pDbgStats)
        m_pDbgStats->vReposition(m_x + m_cx, m_y,
                                 m_cxDesktop - m_cx - m_x + m_xDesktop, m_cyDesktop - m_y + m_yDesktop,
                                 fResize);
}


int
VPoxDbgGui::showConsole()
{
    if (!m_pDbgConsole)
    {
        IVirtualPox *pVirtualPox = NULL;
        m_pMachine->COMGETTER(Parent)(&pVirtualPox);
        m_pDbgConsole = new VPoxDbgConsole(this, m_pParent, pVirtualPox);
        connect(m_pDbgConsole, SIGNAL(destroyed(QObject *)), this, SLOT(notifyChildDestroyed(QObject *)));
        repositionConsole();
    }

    m_pDbgConsole->vShow();
    return VINF_SUCCESS;
}


void
VPoxDbgGui::repositionConsole(bool fResize/* = true*/)
{
    /*
     * Move it to the bottom of the VPox console,
     * and resize it to cover the space down to the bottom of the desktop.
     */
    if (m_pDbgConsole)
        m_pDbgConsole->vReposition(m_x, m_y + m_cy,
                                   RT_MAX(m_cx, 32), m_cyDesktop - m_cy - m_y + m_yDesktop,
                                   fResize);
}


void
VPoxDbgGui::updateDesktopSize()
{
    QRect Rct(0, 0, 1600, 1200);
    QDesktopWidget *pDesktop = QApplication::desktop();
    if (pDesktop)
        Rct = pDesktop->availableGeometry(QPoint(m_x, m_y));
    m_xDesktop = Rct.x();
    m_yDesktop = Rct.y();
    m_cxDesktop = Rct.width();
    m_cyDesktop = Rct.height();
}


void
VPoxDbgGui::adjustRelativePos(int x, int y, unsigned cx, unsigned cy)
{
    /* Disregard a width less than 640 since it will mess up the console. */
    if (cx < 640)
        cx = m_cx;

    const bool fResize = cx != m_cx || cy != m_cy;
    const bool fMoved  = x  != m_x  || y  != m_y;

    m_x = x;
    m_y = y;
    m_cx = cx;
    m_cy = cy;

    if (fMoved)
        updateDesktopSize();
    repositionConsole(fResize);
    repositionStatistics(fResize);
}


QString
VPoxDbgGui::getMachineName() const
{
    QString strName;
    AssertReturn(m_pMachine, strName);
    BSTR bstr;
    HRESULT hrc = m_pMachine->COMGETTER(Name)(&bstr);
    if (SUCCEEDED(hrc))
    {
        strName = QString::fromUtf16(bstr);
        SysFreeString(bstr);
    }
    return strName;
}


void
VPoxDbgGui::notifyChildDestroyed(QObject *pObj)
{
    if (m_pDbgStats == pObj)
        m_pDbgStats = NULL;
    else if (m_pDbgConsole == pObj)
        m_pDbgConsole = NULL;
}

