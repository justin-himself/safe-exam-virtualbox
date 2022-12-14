/* $Id: UIErrorString.cpp $ */
/** @file
 * VPox Qt GUI - UIErrorString class implementation.
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

/* Qt includes: */
#include <QApplication>
#include <QObject>

/* GUI includes: */
#include "UICommon.h"
#include "UIErrorString.h"

/* COM includes: */
#include "COMDefs.h"
#include "CProgress.h"
#include "CVirtualPoxErrorInfo.h"


/* static */
QString UIErrorString::formatRC(HRESULT rc)
{
    QString str;

    PCRTCOMERRMSG msg = NULL;
    const char *pErrMsg = NULL;

    /* First, try as is (only set bit 31 bit for warnings): */
    if (SUCCEEDED_WARNING(rc))
        msg = RTErrCOMGet(rc | 0x80000000);
    else
        msg = RTErrCOMGet(rc);

    if (msg != NULL)
        pErrMsg = msg->pszDefine;

#ifdef VPOX_WS_WIN
    PCRTWINERRMSG winMsg = NULL;

    /* If not found, try again using RTErrWinGet with masked off top 16bit: */
    if (msg == NULL)
    {
        winMsg = RTErrWinGet(rc & 0xFFFF);

        if (winMsg != NULL)
            pErrMsg = winMsg->pszDefine;
    }
#endif /* VPOX_WS_WIN */

    if (pErrMsg != NULL && *pErrMsg != '\0')
        str.sprintf("%s", pErrMsg);

    return str;
}

/* static */
QString UIErrorString::formatRCFull(HRESULT rc)
{
    QString str;

    PCRTCOMERRMSG msg = NULL;
    const char *pErrMsg = NULL;

    /* First, try as is (only set bit 31 bit for warnings): */
    if (SUCCEEDED_WARNING(rc))
        msg = RTErrCOMGet(rc | 0x80000000);
    else
        msg = RTErrCOMGet(rc);

    if (msg != NULL)
        pErrMsg = msg->pszDefine;

#ifdef VPOX_WS_WIN
    PCRTWINERRMSG winMsg = NULL;

    /* If not found, try again using RTErrWinGet with masked off top 16bit: */
    if (msg == NULL)
    {
        winMsg = RTErrWinGet(rc & 0xFFFF);

        if (winMsg != NULL)
            pErrMsg = winMsg->pszDefine;
    }
#endif /* VPOX_WS_WIN */

    if (pErrMsg != NULL && *pErrMsg != '\0')
        str.sprintf("%s (0x%08X)", pErrMsg, rc);
    else
        str.sprintf("0x%08X", rc);

    return str;
}

/* static */
QString UIErrorString::formatErrorInfo(const CProgress &comProgress)
{
    /* Check for API errors first: */
    if (!comProgress.isOk())
        return formatErrorInfo(static_cast<COMBaseWithEI>(comProgress));

    /* For progress errors otherwise: */
    CVirtualPoxErrorInfo comErrorInfo = comProgress.GetErrorInfo();
    /* Handle valid error-info first: */
    if (!comErrorInfo.isNull())
        return formatErrorInfo(comErrorInfo);
    /* Handle NULL error-info otherwise: */
    return QString("<table bgcolor=#EEEEEE border=0 cellspacing=5 cellpadding=0 width=100%>"
                   "<tr><td>%1</td><td><tt>%2</tt></td></tr></table>")
                   .arg(QApplication::translate("UIErrorString", "Result&nbsp;Code: ", "error info"))
                   .arg(formatRCFull(comProgress.GetResultCode()))
                   .prepend("<!--EOM-->") /* move to details */;
}

/* static */
QString UIErrorString::formatErrorInfo(const COMErrorInfo &comInfo, HRESULT wrapperRC /* = S_OK */)
{
    return QString("<qt>%1</qt>").arg(UIErrorString::errorInfoToString(comInfo, wrapperRC));
}

/* static */
QString UIErrorString::formatErrorInfo(const CVirtualPoxErrorInfo &comInfo)
{
    return formatErrorInfo(COMErrorInfo(comInfo));
}

/* static */
QString UIErrorString::formatErrorInfo(const COMBaseWithEI &comWrapper)
{
    Assert(comWrapper.lastRC() != S_OK);
    return formatErrorInfo(comWrapper.errorInfo(), comWrapper.lastRC());
}

/* static */
QString UIErrorString::formatErrorInfo(const COMResult &comRc)
{
    Assert(comRc.rc() != S_OK);
    return formatErrorInfo(comRc.errorInfo(), comRc.rc());
}

/* static */
QString UIErrorString::simplifiedErrorInfo(const COMErrorInfo &comInfo, HRESULT wrapperRC /* = S_OK */)
{
    return UIErrorString::errorInfoToSimpleString(comInfo, wrapperRC);
}

/* static */
QString UIErrorString::simplifiedErrorInfo(const COMBaseWithEI &comWrapper)
{
    Assert(comWrapper.lastRC() != S_OK);
    return simplifiedErrorInfo(comWrapper.errorInfo(), comWrapper.lastRC());
}

/* static */
QString UIErrorString::errorInfoToString(const COMErrorInfo &comInfo, HRESULT wrapperRC)
{
    /* Compose complex details string with internal <!--EOM--> delimiter to
     * make it possible to split string into info & details parts which will
     * be used separately in QIMessageBox. */
    QString strFormatted;

    /* Check if details text is NOT empty: */
    const QString strDetailsInfo = comInfo.text();
    if (!strDetailsInfo.isEmpty())
    {
        /* Check if details text written in English (latin1) and translated: */
        if (   strDetailsInfo == QString::fromLatin1(strDetailsInfo.toLatin1())
            && strDetailsInfo != QObject::tr(strDetailsInfo.toLatin1().constData()))
            strFormatted += QString("<p>%1.</p>").arg(uiCommon().emphasize(QObject::tr(strDetailsInfo.toLatin1().constData())));
        else
            strFormatted += QString("<p>%1.</p>").arg(uiCommon().emphasize(strDetailsInfo));
    }

    strFormatted += "<!--EOM--><table bgcolor=#EEEEEE border=0 cellspacing=5 "
                    "cellpadding=0 width=100%>";

    bool fHaveResultCode = false;

    if (comInfo.isBasicAvailable())
    {
#ifdef VPOX_WS_WIN
        fHaveResultCode = comInfo.isFullAvailable();
        bool fHaveComponent = true;
        bool fHaveInterfaceID = true;
#else /* !VPOX_WS_WIN */
        fHaveResultCode = true;
        bool fHaveComponent = comInfo.isFullAvailable();
        bool fHaveInterfaceID = comInfo.isFullAvailable();
#endif

        if (fHaveResultCode)
        {
            strFormatted += QString("<tr><td>%1</td><td><tt>%2</tt></td></tr>")
                .arg(QApplication::translate("UIErrorString", "Result&nbsp;Code: ", "error info"))
                .arg(formatRCFull(comInfo.resultCode()));
        }

        if (fHaveComponent)
            strFormatted += QString("<tr><td>%1</td><td>%2</td></tr>")
                .arg(QApplication::translate("UIErrorString", "Component: ", "error info"), comInfo.component());

        if (fHaveInterfaceID)
        {
            QString s = comInfo.interfaceID().toString();
            if (!comInfo.interfaceName().isEmpty())
                s = comInfo.interfaceName() + ' ' + s;
            strFormatted += QString("<tr><td>%1</td><td>%2</td></tr>")
                .arg(QApplication::translate("UIErrorString", "Interface: ", "error info"), s);
        }

        if (!comInfo.calleeIID().isNull() && comInfo.calleeIID() != comInfo.interfaceID())
        {
            QString s = comInfo.calleeIID().toString();
            if (!comInfo.calleeName().isEmpty())
                s = comInfo.calleeName() + ' ' + s;
            strFormatted += QString("<tr><td>%1</td><td>%2</td></tr>")
                .arg(QApplication::translate("UIErrorString", "Callee: ", "error info"), s);
        }
    }

    if (   FAILED(wrapperRC)
        && (!fHaveResultCode || wrapperRC != comInfo.resultCode()))
    {
        strFormatted += QString("<tr><td>%1</td><td><tt>%2</tt></td></tr>")
            .arg(QApplication::translate("UIErrorString", "Callee&nbsp;RC: ", "error info"))
            .arg(formatRCFull(wrapperRC));
    }

    strFormatted += "</table>";

    if (comInfo.next())
        strFormatted = strFormatted + "<!--EOP-->" + errorInfoToString(*comInfo.next());

    return strFormatted;
}

/* static */
QString UIErrorString::errorInfoToSimpleString(const COMErrorInfo &comInfo, HRESULT wrapperRC /* = S_OK */)
{
    /* Compose complex details string with text and status code: */
    QString strFormatted;

    /* Check if details text is NOT empty: */
    const QString strDetailsInfo = comInfo.text();
    if (!strDetailsInfo.isEmpty())
        strFormatted += strDetailsInfo;

    /* Check if we have result code: */
    bool fHaveResultCode = false;

    if (comInfo.isBasicAvailable())
    {
#ifdef VPOX_WS_WIN
        fHaveResultCode = comInfo.isFullAvailable();
#else
        fHaveResultCode = true;
#endif

        if (fHaveResultCode)
            strFormatted += "; " + QString("Result Code: ") + formatRCFull(comInfo.resultCode());
    }

    if (   FAILED(wrapperRC)
        && (!fHaveResultCode || wrapperRC != comInfo.resultCode()))
        strFormatted += "; " + QString("Callee RC: ") + formatRCFull(wrapperRC);

    /* Check if we have next error queued: */
    if (comInfo.next())
        strFormatted += "; " + errorInfoToSimpleString(*comInfo.next());

    return strFormatted;
}
