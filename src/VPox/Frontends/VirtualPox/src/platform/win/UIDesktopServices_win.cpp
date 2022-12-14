/* $Id: UIDesktopServices_win.cpp $ */
/** @file
 * VPox Qt GUI - Qt GUI - Utility Classes and Functions specific to Windows..
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

/* VPox includes */
#include "UIDesktopServices.h"

/* Qt includes */
#include <QDir>
#include <QCoreApplication>
#include <QUuid>

/* System includes */
#include <iprt/win/shlobj.h>


bool UIDesktopServices::createMachineShortcut(const QString & /* strSrcFile */, const QString &strDstPath, const QString &strName, const QUuid &uUuid)
{
    IShellLink *pShl = NULL;
    IPersistFile *pPPF = NULL;
    const QString strVPox = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/" + VPOX_GUI_VMRUNNER_IMAGE);
    QFileInfo fi(strVPox);
    QString strVPoxDir = QDir::toNativeSeparators(fi.absolutePath());
    HRESULT rc = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)(&pShl));
    if (FAILED(rc))
        return false;
    do
    {
        rc = pShl->SetPath(strVPox.utf16());
        if (FAILED(rc))
            break;
        rc = pShl->SetWorkingDirectory(strVPoxDir.utf16());
        if (FAILED(rc))
            break;
        QString strArgs = QString("--comment \"%1\" --startvm \"%2\"").arg(strName).arg(uUuid.toString());
        rc = pShl->SetArguments(strArgs.utf16());
        if (FAILED(rc))
            break;
        QString strDesc = QString("Starts the VirtualPox machine %1").arg(strName);
        rc = pShl->SetDescription(strDesc.utf16());
        if (FAILED(rc))
            break;
        rc = pShl->QueryInterface(IID_IPersistFile, (void**)&pPPF);
        if (FAILED(rc))
            break;
        QString strLink = QString("%1\\%2.lnk").arg(strDstPath).arg(strName);
        rc = pPPF->Save(strLink.utf16(), TRUE);
    } while(0);
    if (pPPF)
        pPPF->Release();
    if (pShl)
        pShl->Release();
    return SUCCEEDED(rc);
}

bool UIDesktopServices::openInFileManager(const QString &strFile)
{
    QFileInfo fi(strFile);
    QString strTmp = QDir::toNativeSeparators(fi.absolutePath());

    intptr_t rc = (intptr_t)ShellExecute(NULL, L"explore", strTmp.utf16(), NULL, NULL, SW_SHOWNORMAL);

    return rc > 32 ? true : false;
}

