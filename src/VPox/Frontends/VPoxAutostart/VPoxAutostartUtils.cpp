/* $Id: VPoxAutostartUtils.cpp $ */
/** @file
 * VPoxAutostart - VirtualPox Autostart service, start machines during system boot.
 *                 Utils used by the windows and posix frontends.
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

#include <VPox/com/com.h>
#include <VPox/com/string.h>
#include <VPox/com/Guid.h>
#include <VPox/com/array.h>
#include <VPox/com/ErrorInfo.h>
#include <VPox/com/errorprint.h>

#include <VPox/err.h>

#include <iprt/message.h>
#include <iprt/thread.h>
#include <iprt/stream.h>
#include <iprt/log.h>
#include <iprt/path.h>

#include <algorithm>
#include <list>
#include <string>

#include "VPoxAutostart.h"

using namespace com;

DECLHIDDEN(const char *) machineStateToName(MachineState_T machineState, bool fShort)
{
    switch (machineState)
    {
        case MachineState_PoweredOff:
            return fShort ? "poweroff"             : "powered off";
        case MachineState_Saved:
            return "saved";
        case MachineState_Teleported:
            return "teleported";
        case MachineState_Aborted:
            return "aborted";
        case MachineState_Running:
            return "running";
        case MachineState_Paused:
            return "paused";
        case MachineState_Stuck:
            return fShort ? "gurumeditation"       : "guru meditation";
        case MachineState_Teleporting:
            return "teleporting";
        case MachineState_LiveSnapshotting:
            return fShort ? "livesnapshotting"     : "live snapshotting";
        case MachineState_Starting:
            return "starting";
        case MachineState_Stopping:
            return "stopping";
        case MachineState_Saving:
            return "saving";
        case MachineState_Restoring:
            return "restoring";
        case MachineState_TeleportingPausedVM:
            return fShort ? "teleportingpausedvm"  : "teleporting paused vm";
        case MachineState_TeleportingIn:
            return fShort ? "teleportingin"        : "teleporting (incoming)";
        case MachineState_DeletingSnapshotOnline:
            return fShort ? "deletingsnapshotlive" : "deleting snapshot live";
        case MachineState_DeletingSnapshotPaused:
            return fShort ? "deletingsnapshotlivepaused" : "deleting snapshot live paused";
        case MachineState_OnlineSnapshotting:
            return fShort ? "onlinesnapshotting"   : "online snapshotting";
        case MachineState_RestoringSnapshot:
            return fShort ? "restoringsnapshot"    : "restoring snapshot";
        case MachineState_DeletingSnapshot:
            return fShort ? "deletingsnapshot"     : "deleting snapshot";
        case MachineState_SettingUp:
            return fShort ? "settingup"           : "setting up";
        case MachineState_Snapshotting:
            return "snapshotting";
        default:
            break;
    }
    return "unknown";
}

DECLHIDDEN(RTEXITCODE) autostartSvcLogErrorV(const char *pszFormat, va_list va)
{
    if (*pszFormat)
    {
        char *pszMsg = NULL;
        if (RTStrAPrintfV(&pszMsg, pszFormat, va) != -1)
        {
            autostartSvcOsLogStr(pszMsg, AUTOSTARTLOGTYPE_ERROR);
            RTStrFree(pszMsg);
        }
        else
            autostartSvcOsLogStr(pszFormat, AUTOSTARTLOGTYPE_ERROR);
    }
    return RTEXITCODE_FAILURE;
}

DECLHIDDEN(RTEXITCODE) autostartSvcLogError(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    autostartSvcLogErrorV(pszFormat, va);
    va_end(va);
    return RTEXITCODE_FAILURE;
}

DECLHIDDEN(void) autostartSvcLogVerboseV(const char *pszFormat, va_list va)
{
    if (*pszFormat)
    {
        char *pszMsg = NULL;
        if (RTStrAPrintfV(&pszMsg, pszFormat, va) != -1)
        {
            autostartSvcOsLogStr(pszMsg, AUTOSTARTLOGTYPE_VERBOSE);
            RTStrFree(pszMsg);
        }
        else
            autostartSvcOsLogStr(pszFormat, AUTOSTARTLOGTYPE_VERBOSE);
    }
}

DECLHIDDEN(void) autostartSvcLogVerbose(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    autostartSvcLogVerboseV(pszFormat, va);
    va_end(va);
}

DECLHIDDEN(void) autostartSvcLogWarningV(const char *pszFormat, va_list va)
{
    if (*pszFormat)
    {
        char *pszMsg = NULL;
        if (RTStrAPrintfV(&pszMsg, pszFormat, va) != -1)
        {
            autostartSvcOsLogStr(pszMsg, AUTOSTARTLOGTYPE_WARNING);
            RTStrFree(pszMsg);
        }
        else
            autostartSvcOsLogStr(pszFormat, AUTOSTARTLOGTYPE_WARNING);
    }
}

DECLHIDDEN(void) autostartSvcLogInfo(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    autostartSvcLogInfoV(pszFormat, va);
    va_end(va);
}

DECLHIDDEN(void) autostartSvcLogInfoV(const char *pszFormat, va_list va)
{
    if (*pszFormat)
    {
        char *pszMsg = NULL;
        if (RTStrAPrintfV(&pszMsg, pszFormat, va) != -1)
        {
            autostartSvcOsLogStr(pszMsg, AUTOSTARTLOGTYPE_INFO);
            RTStrFree(pszMsg);
        }
        else
            autostartSvcOsLogStr(pszFormat, AUTOSTARTLOGTYPE_INFO);
    }
}

DECLHIDDEN(void) autostartSvcLogWarning(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    autostartSvcLogWarningV(pszFormat, va);
    va_end(va);
}

DECLHIDDEN(RTEXITCODE) autostartSvcLogGetOptError(const char *pszAction, int rc, int argc, char **argv, int iArg, PCRTGETOPTUNION pValue)
{
    NOREF(pValue);
    autostartSvcLogError("%s - RTGetOpt failure, %Rrc (%d): %s",
                   pszAction, rc, rc, iArg < argc ? argv[iArg] : "<null>");
    return RTEXITCODE_FAILURE;
}

DECLHIDDEN(RTEXITCODE) autostartSvcLogTooManyArgsError(const char *pszAction, int argc, char **argv, int iArg)
{
    Assert(iArg < argc);
    autostartSvcLogError("%s - Too many arguments: %s", pszAction, argv[iArg]);
    for ( ; iArg < argc; iArg++)
        LogRel(("arg#%i: %s\n", iArg, argv[iArg]));
    return RTEXITCODE_FAILURE;
}

DECLHIDDEN(RTEXITCODE) autostartSvcDisplayErrorV(const char *pszFormat, va_list va)
{
    RTStrmPrintf(g_pStdErr, "VPoxSupSvc error: ");
    RTStrmPrintfV(g_pStdErr, pszFormat, va);
    Log(("autostartSvcDisplayErrorV: %s", pszFormat)); /** @todo format it! */
    return RTEXITCODE_FAILURE;
}

DECLHIDDEN(RTEXITCODE) autostartSvcDisplayError(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    autostartSvcDisplayErrorV(pszFormat, va);
    va_end(va);
    return RTEXITCODE_FAILURE;
}

DECLHIDDEN(RTEXITCODE) autostartSvcDisplayGetOptError(const char *pszAction, int rc, PCRTGETOPTUNION pValue)
{
    char szMsg[4096];
    RTGetOptFormatError(szMsg, sizeof(szMsg), rc, pValue);
    autostartSvcDisplayError("%s - %s", pszAction, szMsg);
    return RTEXITCODE_SYNTAX;
}

DECLHIDDEN(int) autostartSetup()
{
    autostartSvcOsLogStr("Setting up ...\n", AUTOSTARTLOGTYPE_VERBOSE);

    /*
     * Initialize COM.
     */
    using namespace com;
    HRESULT hrc = com::Initialize();
# ifdef VPOX_WITH_XPCOM
    if (hrc == NS_ERROR_FILE_ACCESS_DENIED)
    {
        char szHome[RTPATH_MAX] = "";
        com::GetVPoxUserHomeDirectory(szHome, sizeof(szHome));
        return RTMsgErrorExit(RTEXITCODE_FAILURE,
               "Failed to initialize COM because the global settings directory '%s' is not accessible!", szHome);
    }
# endif
    if (FAILED(hrc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to initialize COM (%Rhrc)!", hrc);

    hrc = g_pVirtualPoxClient.createInprocObject(CLSID_VirtualPoxClient);
    if (FAILED(hrc))
    {
        RTMsgError("Failed to create the VirtualPoxClient object (%Rhrc)!", hrc);
        com::ErrorInfo info;
        if (!info.isFullAvailable() && !info.isBasicAvailable())
        {
            com::GluePrintRCMessage(hrc);
            RTMsgError("Most likely, the VirtualPox COM server is not running or failed to start.");
        }
        else
            com::GluePrintErrorInfo(info);
        return RTEXITCODE_FAILURE;
    }

    /*
     * Setup VirtualPox + session interfaces.
     */
    HRESULT rc = g_pVirtualPoxClient->COMGETTER(VirtualPox)(g_pVirtualPox.asOutParam());
    if (SUCCEEDED(rc))
    {
        rc = g_pSession.createInprocObject(CLSID_Session);
        if (FAILED(rc))
            RTMsgError("Failed to create a session object (rc=%Rhrc)!", rc);
    }
    else
        RTMsgError("Failed to get VirtualPox object (rc=%Rhrc)!", rc);

    if (FAILED(rc))
        return VERR_COM_OBJECT_NOT_FOUND;

    return VINF_SUCCESS;
}

DECLHIDDEN(void) autostartShutdown()
{
    autostartSvcOsLogStr("Shutting down ...\n", AUTOSTARTLOGTYPE_VERBOSE);

    g_pSession.setNull();
    g_pVirtualPox.setNull();
    g_pVirtualPoxClient.setNull();
    com::Shutdown();
}

