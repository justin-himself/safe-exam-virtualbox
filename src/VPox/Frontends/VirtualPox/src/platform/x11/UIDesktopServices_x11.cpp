/* $Id: UIDesktopServices_x11.cpp $ */
/** @file
 * VPox Qt GUI - Qt GUI - Utility Classes and Functions specific to X11..
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
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QUrl>


bool UIDesktopServices::createMachineShortcut(const QString & /* strSrcFile */, const QString &strDstPath, const QString &strName, const QUuid &uUuid)
{
    QFile link(strDstPath + QDir::separator() + strName + ".desktop");
    if (link.open(QFile::WriteOnly | QFile::Truncate))
    {
        const QString strVPox = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/" + VPOX_GUI_VMRUNNER_IMAGE);
        QTextStream out(&link);
        out.setCodec("UTF-8");
        /* Create a link which starts VirtualPox with the machine uuid. */
        out << "[Desktop Entry]" << endl
            << "Encoding=UTF-8" << endl
            << "Version=1.0" << endl
            << "Name=" << strName << endl
            << "Comment=Starts the VirtualPox machine " << strName << endl
            << "Type=Application" << endl
            << "Exec=" << strVPox << " --comment \"" << strName << "\" --startvm \"" << uUuid.toString() << "\"" << endl
            << "Icon=virtualpox-vpox.png" << endl;
        /* This would be a real file link entry, but then we could also simply
         * use a soft link (on most UNIX fs):
        out << "[Desktop Entry]" << endl
            << "Encoding=UTF-8" << endl
            << "Version=1.0" << endl
            << "Name=" << strName << endl
            << "Type=Link" << endl
            << "Icon=virtualpox-vpox.png" << endl
        */
        link.setPermissions(link.permissions() | QFile::ExeOwner);
        return true;
    }
    return false;
}

bool UIDesktopServices::openInFileManager(const QString &strFile)
{
    QFileInfo fi(strFile);
    return QDesktopServices::openUrl(QUrl("file://" + QDir::toNativeSeparators(fi.absolutePath()), QUrl::TolerantMode));
}

