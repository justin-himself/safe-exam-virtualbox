/* $Id: VPoxInstallHelper.cpp $ */
/** @file
 * VPoxInstallHelper - Various helper routines for Windows host installer.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
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
#ifdef VPOX_WITH_NETFLT
# include "VPox/VPoxNetCfg-win.h"
# include "VPox/VPoxDrvCfg-win.h"
#endif /* VPOX_WITH_NETFLT */

#include <VPox/version.h>

#include <iprt/win/windows.h>
#include <wchar.h>
#include <stdio.h>

#include <msi.h>
#include <msiquery.h>

#define _WIN32_DCOM
#include <iprt/win/windows.h>
#include <assert.h>
#include <shellapi.h>
#define INITGUID
#include <guiddef.h>
#include <devguid.h>
#include <iprt/win/objbase.h>
#include <iprt/win/setupapi.h>
#include <iprt/win/shlobj.h>
#include <cfgmgr32.h>

#include "VPoxCommon.h"

#ifndef VPOX_OSE
# include "internal/VPoxSerial.h"
#endif


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifdef DEBUG
# define NonStandardAssert(_expr) assert(_expr)
#else
# define NonStandardAssert(_expr) do{ }while(0)
#endif

#define MY_WTEXT_HLP(a_str) L##a_str
#define MY_WTEXT(a_str)     MY_WTEXT_HLP(a_str)


BOOL APIENTRY DllMain(HANDLE hModule, DWORD  uReason, LPVOID lpReserved)
{
    RT_NOREF3(hModule, uReason, lpReserved);
    return TRUE;
}


static void logString(MSIHANDLE hInstall, LPCSTR szString, ...)
{
    PMSIHANDLE newHandle = MsiCreateRecord(2);

    char szBuffer[1024];
    va_list va;
    va_start(va, szString);
    _vsnprintf(szBuffer, sizeof(szBuffer), szString, va);
    va_end(va);

    MsiRecordSetStringA(newHandle, 0, szBuffer);
    MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_INFO), newHandle);
    MsiCloseHandle(newHandle);

#if 0/*def DEBUG - wrong? What does '%s' expect, wchar or char? */
    _tprintf(_T("Debug: %s\n"), szBuffer);
#endif
}

static void logStringW(MSIHANDLE hInstall, LPCWSTR pwszString, ...)
{
    PMSIHANDLE newHandle = MsiCreateRecord(2);

    WCHAR szBuffer[1024];
    va_list va;
    va_start(va, pwszString);
    _vsnwprintf(szBuffer, RT_ELEMENTS(szBuffer), pwszString, va);
    va_end(va);

    MsiRecordSetStringW(newHandle, 0, szBuffer);
    MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_INFO), newHandle);
    MsiCloseHandle(newHandle);
}

UINT __stdcall IsSerialCheckNeeded(MSIHANDLE hModule)
{
#ifndef VPOX_OSE
    /*BOOL bRet =*/ serialCheckNeeded(hModule);
#else
    RT_NOREF1(hModule);
#endif
    return ERROR_SUCCESS;
}

UINT __stdcall CheckSerial(MSIHANDLE hModule)
{
#ifndef VPOX_OSE
    /*BOOL bRet =*/ serialIsValid(hModule);
#else
    RT_NOREF1(hModule);
#endif
    return ERROR_SUCCESS;
}

DWORD Exec(MSIHANDLE hModule,
           const WCHAR *pwszAppName, WCHAR *pwszCmdLine, const WCHAR *pwszWorkDir,
           DWORD *pdwExitCode)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DWORD rc = ERROR_SUCCESS;

    ZeroMemory(&si, sizeof(si));
    si.dwFlags = STARTF_USESHOWWINDOW;
#ifdef UNICODE
    si.dwFlags |= CREATE_UNICODE_ENVIRONMENT;
#endif
    si.wShowWindow = SW_HIDE; /* For debugging: SW_SHOW; */
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    logStringW(hModule, L"Executing command line: %s %s (Working Dir: %s)",
               pwszAppName, pwszCmdLine, pwszWorkDir == NULL ? L"Current" : pwszWorkDir);

    SetLastError(0);
    if (!CreateProcessW(pwszAppName,  /* Module name. */
                        pwszCmdLine,  /* Command line. */
                        NULL,         /* Process handle not inheritable. */
                        NULL,         /* Thread handle not inheritable. */
                        FALSE,        /* Set handle inheritance to FALSE .*/
                        0,            /* No creation flags. */
                        NULL,         /* Use parent's environment block. */
                        pwszWorkDir,  /* Use parent's starting directory. */
                        &si,          /* Pointer to STARTUPINFO structure. */
                        &pi))         /* Pointer to PROCESS_INFORMATION structure. */
    {
        rc = GetLastError();
        logStringW(hModule, L"Executing command line: CreateProcess() failed! Error: %ld", rc);
        return rc;
    }

    /* Wait until child process exits. */
    if (WAIT_FAILED == WaitForSingleObject(pi.hProcess, 30 * 1000 /* Wait 30 secs max. */))
    {
        rc = GetLastError();
        logStringW(hModule, L"Executing command line: WaitForSingleObject() failed! Error: %u", rc);
    }
    else
    {
        if (!GetExitCodeProcess(pi.hProcess, pdwExitCode))
        {
            rc = GetLastError();
            logStringW(hModule, L"Executing command line: GetExitCodeProcess() failed! Error: %u", rc);
        }
    }

    /* Close process and thread handles. */
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    logStringW(hModule, L"Executing command returned: %u (exit code %u)", rc, *pdwExitCode);
    return rc;
}

/**
 * Checks if the running OS is (at least) Windows 10 (e.g. >= build 10000).
 *
 * Called from the MSI installer as custom action.
 *
 * @returns Always ERROR_SUCCESS.
 *          Sets public property VPOX_IS_WINDOWS_10 to "" (empty / false) or "1" (success).
 *
 * @param   hModule             Windows installer module handle.
 */
UINT __stdcall IsWindows10(MSIHANDLE hModule)
{
    /*
     * Note: We cannot use RtlGetVersion() / GetVersionExW() here, as the Windows Installer service
     *       all shims this, unfortunately. So we have to go another route by querying the major version
     *       number from the registry.
     */
    HKEY hKeyCurVer = NULL;
    LSTATUS dwErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKeyCurVer);
    if (dwErr == ERROR_SUCCESS)
    {
        DWORD dwVal = 0;
        DWORD cbVal = sizeof(dwVal);
        DWORD dwValueType = REG_DWORD;
        dwErr = RegQueryValueExW(hKeyCurVer, L"CurrentMajorVersionNumber", NULL, &dwValueType, (LPBYTE)&dwVal, &cbVal);
        if (dwErr == ERROR_SUCCESS)
        {
            logStringW(hModule, L"IsWindows10/CurrentMajorVersionNumber: %ld", dwVal);

            VPoxSetProperty(hModule, L"VPOX_IS_WINDOWS_10", dwVal >= 10 ? L"1" : L"");
        }
        else
            logStringW(hModule, L"IsWindows10/RegOpenKeyExW: Error reading CurrentMajorVersionNumber (%ld)", dwErr);

        RegCloseKey(hKeyCurVer);
    }
    else
        logStringW(hModule, L"IsWindows10/RegOpenKeyExW: Error opening CurrentVersion key (%ld)", dwErr);

    return ERROR_SUCCESS; /* Never return failure. */
}

UINT __stdcall InstallPythonAPI(MSIHANDLE hModule)
{
    logStringW(hModule, L"InstallPythonAPI: Checking for installed Python environment ...");

    HKEY hkPythonCore = NULL;
    BOOL bFound = FALSE;
    LONG rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Python\\PythonCore", 0, KEY_READ, &hkPythonCore);
    if (rc != ERROR_SUCCESS)
    {
        logStringW(hModule, L"InstallPythonAPI: Python seems not to be installed.");
        return ERROR_SUCCESS;
    }

    WCHAR wszPath[MAX_PATH] = { 0 };
    WCHAR wszVal[MAX_PATH] = { 0 };

    for (int i = 0;; ++i)
    {
        WCHAR wszRoot[MAX_PATH] = { 0 };
        DWORD dwLen = sizeof(wszPath);
        DWORD dwKeyType = REG_SZ;

        rc = RegEnumKeyExW(hkPythonCore, i, wszRoot, &dwLen, NULL, NULL, NULL, NULL);
        if (rc != ERROR_SUCCESS || dwLen <= 0)
            break;

        swprintf_s(wszPath, RT_ELEMENTS(wszPath), L"%s\\InstallPath", wszRoot);
        dwLen = sizeof(wszVal);

        HKEY hkPythonInstPath = NULL;
        rc = RegOpenKeyExW(hkPythonCore, wszPath, 0, KEY_READ,  &hkPythonInstPath);
        if (rc != ERROR_SUCCESS)
            continue;

        rc = RegQueryValueExW(hkPythonInstPath, L"", NULL, &dwKeyType, (LPBYTE)wszVal, &dwLen);
        if (rc == ERROR_SUCCESS)
            logStringW(hModule, L"InstallPythonAPI: Path \"%s\" detected.", wszVal);

        RegCloseKey(hkPythonInstPath);
    }
    RegCloseKey(hkPythonCore);

    /* Python path found? */
    WCHAR wszExec[MAX_PATH] = { 0 };
    WCHAR wszCmdLine[MAX_PATH] = { 0 };
    DWORD dwExitCode = 0;
    if (wcslen(wszVal) > 0)
    {
        /* Cool, check for installed Win32 extensions. */
        logStringW(hModule, L"InstallPythonAPI: Python installed. Checking for Win32 extensions ...");
        swprintf_s(wszExec, RT_ELEMENTS(wszExec), L"%s\\python.exe", wszVal);
        swprintf_s(wszCmdLine, RT_ELEMENTS(wszCmdLine), L"%s\\python.exe -c \"import win32api\"", wszVal);

        DWORD dwRetExec = Exec(hModule, wszExec, wszCmdLine, NULL, &dwExitCode);
        if (   (ERROR_SUCCESS == dwRetExec)
            && (            0 == dwExitCode))
        {
            /* Did we get the correct error level (=0)? */
            logStringW(hModule, L"InstallPythonAPI: Win32 extensions installed.");
            bFound = TRUE;
        }
        else
            logStringW(hModule, L"InstallPythonAPI: Win32 extensions not found.");
    }

    BOOL bInstalled = FALSE;
    if (bFound) /* Is Python and all required stuff installed? */
    {
        /* Get the VPoxAPI setup string. */
        WCHAR wszPathTargetDir[MAX_PATH] = {0};
        VPoxGetProperty(hModule, L"CustomActionData", wszPathTargetDir, sizeof(wszPathTargetDir));
        if (wcslen(wszPathTargetDir))
        {
            /* Set final path. */
            swprintf_s(wszPath, RT_ELEMENTS(wszPath), L"%s", wszPathTargetDir);

            /* Install our API module. */
            swprintf_s(wszCmdLine, RT_ELEMENTS(wszCmdLine), L"%s\\python.exe vpoxapisetup.py install", wszVal);

            /* Set required environment variables. */
            if (!SetEnvironmentVariableW(L"VPOX_INSTALL_PATH", wszPathTargetDir))
                logStringW(hModule, L"InstallPythonAPI: Could set environment variable VPOX_INSTALL_PATH!");
            else
            {
                DWORD dwRetExec = Exec(hModule, wszExec, wszCmdLine, wszPath, &dwExitCode);
                if (   (ERROR_SUCCESS == dwRetExec)
                    && (            0 == dwExitCode))
                {
                    /* All done! */
                    logStringW(hModule, L"InstallPythonAPI: VPoxAPI for Python successfully installed.");
                    bInstalled = TRUE;
                }
                else
                {
                    if (dwRetExec)
                        logStringW(hModule, L"InstallPythonAPI: Error while executing installation of VPox API: %ld", dwRetExec);
                    else
                        logStringW(hModule, L"InstallPythonAPI: Python reported an error while installing VPox API: %ld", dwExitCode);
                }
            }
        }
        else
            logStringW(hModule, L"InstallPythonAPI: Unable to retrieve VPox installation path!");
    }

    VPoxSetProperty(hModule, L"VPOX_PYTHON_IS_INSTALLED", bInstalled ? L"1" : L"0");

    if (!bInstalled)
        logStringW(hModule, L"InstallPythonAPI: VPox API not installed.");
    return ERROR_SUCCESS; /* Do not fail here. */
}

static LONG installBrandingValue(MSIHANDLE hModule,
                                 const WCHAR *pwszFileName,
                                 const WCHAR *pwszSection,
                                 const WCHAR *pwszValue)
{
    LONG rc;
    WCHAR wszValue[_MAX_PATH];
    if (GetPrivateProfileStringW(pwszSection, pwszValue, NULL,
                                 wszValue, sizeof(wszValue), pwszFileName) > 0)
    {
        HKEY hkBranding;
        WCHAR wszKey[_MAX_PATH];

        if (wcsicmp(L"General", pwszSection) != 0)
            swprintf_s(wszKey, RT_ELEMENTS(wszKey), L"SOFTWARE\\%s\\VirtualPox\\Branding\\%s", VPOX_VENDOR_SHORT, pwszSection);
        else
            swprintf_s(wszKey, RT_ELEMENTS(wszKey), L"SOFTWARE\\%s\\VirtualPox\\Branding", VPOX_VENDOR_SHORT);

        rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszKey, 0, KEY_WRITE, &hkBranding);
        if (rc == ERROR_SUCCESS)
        {
            rc = RegSetValueExW(hkBranding,
                                pwszValue,
                                NULL,
                                REG_SZ,
                                (BYTE *)wszValue,
                                (DWORD)wcslen(wszValue));
            if (rc != ERROR_SUCCESS)
                logStringW(hModule, L"InstallBranding: Could not write value %s! Error %ld", pwszValue, rc);
            RegCloseKey (hkBranding);
        }
    }
    else
        rc = ERROR_NOT_FOUND;
    return rc;
}

UINT CopyDir(MSIHANDLE hModule, const WCHAR *pwszDestDir, const WCHAR *pwszSourceDir)
{
    UINT rc;
    WCHAR wszDest[_MAX_PATH + 1];
    WCHAR wszSource[_MAX_PATH + 1];

    swprintf_s(wszDest, RT_ELEMENTS(wszDest), L"%s%c", pwszDestDir, L'\0');
    swprintf_s(wszSource, RT_ELEMENTS(wszSource), L"%s%c", pwszSourceDir, L'\0');

    SHFILEOPSTRUCTW s = {0};
    s.hwnd = NULL;
    s.wFunc = FO_COPY;
    s.pTo = wszDest;
    s.pFrom = wszSource;
    s.fFlags = FOF_SILENT |
               FOF_NOCONFIRMATION |
               FOF_NOCONFIRMMKDIR |
               FOF_NOERRORUI;

    logStringW(hModule, L"CopyDir: DestDir=%s, SourceDir=%s", wszDest, wszSource);
    int r = SHFileOperationW(&s);
    if (r != 0)
    {
        logStringW(hModule, L"CopyDir: Copy operation returned status 0x%x", r);
        rc = ERROR_GEN_FAILURE;
    }
    else
        rc = ERROR_SUCCESS;
    return rc;
}

UINT RemoveDir(MSIHANDLE hModule, const WCHAR *pwszDestDir)
{
    UINT rc;
    WCHAR wszDest[_MAX_PATH + 1];

    swprintf_s(wszDest, RT_ELEMENTS(wszDest), L"%s%c", pwszDestDir, L'\0');

    SHFILEOPSTRUCTW s = {0};
    s.hwnd = NULL;
    s.wFunc = FO_DELETE;
    s.pFrom = wszDest;
    s.fFlags = FOF_SILENT
             | FOF_NOCONFIRMATION
             | FOF_NOCONFIRMMKDIR
             | FOF_NOERRORUI;

    logStringW(hModule, L"RemoveDir: DestDir=%s", wszDest);
    int r = SHFileOperationW(&s);
    if (r != 0)
    {
        logStringW(hModule, L"RemoveDir: Remove operation returned status 0x%x", r);
        rc = ERROR_GEN_FAILURE;
    }
    else
        rc = ERROR_SUCCESS;
    return rc;
}

UINT RenameDir(MSIHANDLE hModule, const WCHAR *pwszDestDir, const WCHAR *pwszSourceDir)
{
    UINT rc;
    WCHAR wszDest[_MAX_PATH + 1];
    WCHAR wszSource[_MAX_PATH + 1];

    swprintf_s(wszDest, RT_ELEMENTS(wszDest), L"%s%c", pwszDestDir, L'\0');
    swprintf_s(wszSource, RT_ELEMENTS(wszSource), L"%s%c", pwszSourceDir, L'\0');

    SHFILEOPSTRUCTW s = {0};
    s.hwnd = NULL;
    s.wFunc = FO_RENAME;
    s.pTo = wszDest;
    s.pFrom = wszSource;
    s.fFlags = FOF_SILENT
             | FOF_NOCONFIRMATION
             | FOF_NOCONFIRMMKDIR
             | FOF_NOERRORUI;

    logStringW(hModule, L"RenameDir: DestDir=%s, SourceDir=%s", wszDest, wszSource);
    int r = SHFileOperationW(&s);
    if (r != 0)
    {
        logStringW(hModule, L"RenameDir: Rename operation returned status 0x%x", r);
        rc = ERROR_GEN_FAILURE;
    }
    else
        rc = ERROR_SUCCESS;
    return rc;
}

UINT __stdcall UninstallBranding(MSIHANDLE hModule)
{
    UINT rc;
    logStringW(hModule, L"UninstallBranding: Handling branding file ...");

    WCHAR wszPathTargetDir[_MAX_PATH];
    WCHAR wszPathDest[_MAX_PATH];

    rc = VPoxGetProperty(hModule, L"CustomActionData", wszPathTargetDir, sizeof(wszPathTargetDir));
    if (rc == ERROR_SUCCESS)
    {
        /** @todo Check trailing slash after %s. */
/** @todo r=bird: using swprintf_s for formatting paths without checking for
 * overflow not good.  You're dodging the buffer overflow issue only to end up
 * with incorrect behavior!  (Truncated file/dir names)
 *
 * This applies almost to all swprintf_s calls in this file!!
 */
        swprintf_s(wszPathDest, RT_ELEMENTS(wszPathDest), L"%scustom", wszPathTargetDir);
        rc = RemoveDir(hModule, wszPathDest);
        if (rc != ERROR_SUCCESS)
        {
            /* Check for hidden .custom directory and remove it. */
            swprintf_s(wszPathDest, RT_ELEMENTS(wszPathDest), L"%s.custom", wszPathTargetDir);
            rc = RemoveDir(hModule, wszPathDest);
        }
    }

    logStringW(hModule, L"UninstallBranding: Handling done. (rc=%u (ignored))", rc);
    return ERROR_SUCCESS; /* Do not fail here. */
}

UINT __stdcall InstallBranding(MSIHANDLE hModule)
{
    UINT rc;
    logStringW(hModule, L"InstallBranding: Handling branding file ...");

    WCHAR wszPathMSI[_MAX_PATH];
    rc = VPoxGetProperty(hModule, L"SOURCEDIR", wszPathMSI, sizeof(wszPathMSI));
    if (rc == ERROR_SUCCESS)
    {
        WCHAR wszPathTargetDir[_MAX_PATH];
        rc = VPoxGetProperty(hModule, L"CustomActionData", wszPathTargetDir, sizeof(wszPathTargetDir));
        if (rc == ERROR_SUCCESS)
        {
            /** @todo Check for trailing slash after %s. */
            WCHAR wszPathDest[_MAX_PATH];
            swprintf_s(wszPathDest, RT_ELEMENTS(wszPathDest), L"%s", wszPathTargetDir);

            WCHAR wszPathSource[_MAX_PATH];
            swprintf_s(wszPathSource, RT_ELEMENTS(wszPathSource), L"%s.custom", wszPathMSI);
            rc = CopyDir(hModule, wszPathDest, wszPathSource);
            if (rc == ERROR_SUCCESS)
            {
                swprintf_s(wszPathDest, RT_ELEMENTS(wszPathDest), L"%scustom", wszPathTargetDir);
                swprintf_s(wszPathSource, RT_ELEMENTS(wszPathSource), L"%s.custom", wszPathTargetDir);
                rc = RenameDir(hModule, wszPathDest, wszPathSource);
            }
        }
    }

    logStringW(hModule, L"InstallBranding: Handling done. (rc=%u (ignored))", rc);
    return ERROR_SUCCESS; /* Do not fail here. */
}

#ifdef VPOX_WITH_NETFLT

/** @todo should use some real VPox app name */
#define VPOX_NETCFG_APP_NAME L"VirtualPox Installer"
#define VPOX_NETCFG_MAX_RETRIES 10
#define NETFLT_PT_INF_REL_PATH L"VPoxNetFlt.inf"
#define NETFLT_MP_INF_REL_PATH L"VPoxNetFltM.inf"
#define NETFLT_ID  L"sun_VPoxNetFlt" /** @todo Needs to be changed (?). */
#define NETADP_ID  L"sun_VPoxNetAdp" /** @todo Needs to be changed (?). */

#define NETLWF_INF_NAME L"VPoxNetLwf.inf"

static MSIHANDLE g_hCurrentModule = NULL;

static UINT _uninstallNetFlt(MSIHANDLE hModule);
static UINT _uninstallNetLwf(MSIHANDLE hModule);

static VOID vpoxDrvLoggerCallback(VPOXDRVCFG_LOG_SEVERITY enmSeverity, char *pszMsg, void *pvContext)
{
    RT_NOREF1(pvContext);
    switch (enmSeverity)
    {
        case VPOXDRVCFG_LOG_SEVERITY_FLOW:
        case VPOXDRVCFG_LOG_SEVERITY_REGULAR:
            break;
        case VPOXDRVCFG_LOG_SEVERITY_REL:
            if (g_hCurrentModule)
                logString(g_hCurrentModule, pszMsg);
            break;
        default:
            break;
    }
}

static VOID netCfgLoggerCallback(LPCSTR szString)
{
    if (g_hCurrentModule)
        logString(g_hCurrentModule, szString);
}

static VOID netCfgLoggerDisable()
{
    if (g_hCurrentModule)
    {
        VPoxNetCfgWinSetLogging((LOG_ROUTINE)NULL);
        g_hCurrentModule = NULL;
    }
}

static VOID netCfgLoggerEnable(MSIHANDLE hModule)
{
    NonStandardAssert(hModule);

    if (g_hCurrentModule)
        netCfgLoggerDisable();

    g_hCurrentModule = hModule;

    VPoxNetCfgWinSetLogging((LOG_ROUTINE)netCfgLoggerCallback);
    /* uncomment next line if you want to add logging information from VPoxDrvCfg.cpp */
//    VPoxDrvCfgLoggerSet(vpoxDrvLoggerCallback, NULL);
}

static UINT errorConvertFromHResult(MSIHANDLE hModule, HRESULT hr)
{
    UINT uRet;
    switch (hr)
    {
        case S_OK:
            uRet = ERROR_SUCCESS;
            break;

        case NETCFG_S_REBOOT:
        {
            logStringW(hModule, L"Reboot required, setting REBOOT property to \"force\"");
            HRESULT hr2 = MsiSetPropertyW(hModule, L"REBOOT", L"Force");
            if (hr2 != ERROR_SUCCESS)
                logStringW(hModule, L"Failed to set REBOOT property, error = 0x%x", hr2);
            uRet = ERROR_SUCCESS; /* Never fail here. */
            break;
        }

        default:
            logStringW(hModule, L"Converting unhandled HRESULT (0x%x) to ERROR_GEN_FAILURE", hr);
            uRet = ERROR_GEN_FAILURE;
    }
    return uRet;
}

static MSIHANDLE createNetCfgLockedMsgRecord(MSIHANDLE hModule)
{
    MSIHANDLE hRecord = MsiCreateRecord(2);
    if (hRecord)
    {
        UINT uErr = MsiRecordSetInteger(hRecord, 1, 25001);
        if (uErr != ERROR_SUCCESS)
        {
            logStringW(hModule, L"createNetCfgLockedMsgRecord: MsiRecordSetInteger failed, error = 0x%x", uErr);
            MsiCloseHandle(hRecord);
            hRecord = NULL;
        }
    }
    else
        logStringW(hModule, L"createNetCfgLockedMsgRecord: Failed to create a record");

    return hRecord;
}

static UINT doNetCfgInit(MSIHANDLE hModule, INetCfg **ppnc, BOOL bWrite)
{
    MSIHANDLE hMsg = NULL;
    UINT uErr = ERROR_GEN_FAILURE;
    int MsgResult;
    int cRetries = 0;

    do
    {
        LPWSTR lpszLockedBy;
        HRESULT hr = VPoxNetCfgWinQueryINetCfg(ppnc, bWrite, VPOX_NETCFG_APP_NAME, 10000, &lpszLockedBy);
        if (hr != NETCFG_E_NO_WRITE_LOCK)
        {
            if (FAILED(hr))
                logStringW(hModule, L"doNetCfgInit: VPoxNetCfgWinQueryINetCfg failed, error = 0x%x", hr);
            uErr = errorConvertFromHResult(hModule, hr);
            break;
        }

        /* hr == NETCFG_E_NO_WRITE_LOCK */

        if (!lpszLockedBy)
        {
            logStringW(hModule, L"doNetCfgInit: lpszLockedBy == NULL, breaking");
            break;
        }

        /* on vista the 6to4svc.dll periodically maintains the lock for some reason,
         * if this is the case, increase the wait period by retrying multiple times
         * NOTE: we could alternatively increase the wait timeout,
         * however it seems unneeded for most cases, e.g. in case some network connection property
         * dialog is opened, it would be better to post a notification to the user as soon as possible
         * rather than waiting for a longer period of time before displaying it */
        if (   cRetries < VPOX_NETCFG_MAX_RETRIES
            && !wcscmp(lpszLockedBy, L"6to4svc.dll"))
        {
            cRetries++;
            logStringW(hModule, L"doNetCfgInit: lpszLockedBy is 6to4svc.dll, retrying %d out of %d", cRetries, VPOX_NETCFG_MAX_RETRIES);
            MsgResult = IDRETRY;
        }
        else
        {
            if (!hMsg)
            {
                hMsg = createNetCfgLockedMsgRecord(hModule);
                if (!hMsg)
                {
                    logStringW(hModule, L"doNetCfgInit: Failed to create a message record, breaking");
                    CoTaskMemFree(lpszLockedBy);
                    break;
                }
            }

            UINT rTmp = MsiRecordSetStringW(hMsg, 2, lpszLockedBy);
            NonStandardAssert(rTmp == ERROR_SUCCESS);
            if (rTmp != ERROR_SUCCESS)
            {
                logStringW(hModule, L"doNetCfgInit: MsiRecordSetStringW failed, error = 0x%x", rTmp);
                CoTaskMemFree(lpszLockedBy);
                break;
            }

            MsgResult = MsiProcessMessage(hModule, (INSTALLMESSAGE)(INSTALLMESSAGE_USER | MB_RETRYCANCEL), hMsg);
            NonStandardAssert(MsgResult == IDRETRY || MsgResult == IDCANCEL);
            logStringW(hModule, L"doNetCfgInit: MsiProcessMessage returned (0x%x)", MsgResult);
        }
        CoTaskMemFree(lpszLockedBy);
    } while(MsgResult == IDRETRY);

    if (hMsg)
        MsiCloseHandle(hMsg);

    return uErr;
}

static UINT vpoxNetFltQueryInfArray(MSIHANDLE hModule, OUT LPWSTR pwszPtInf, OUT LPWSTR pwszMpInf, DWORD dwSize)
{
    DWORD dwBuf = dwSize - RT_MAX(sizeof(NETFLT_PT_INF_REL_PATH), sizeof(NETFLT_MP_INF_REL_PATH));
    UINT uErr = MsiGetPropertyW(hModule, L"CustomActionData", pwszPtInf, &dwBuf);
    if (   uErr == ERROR_SUCCESS
        && dwBuf)
    {
        wcscpy(pwszMpInf, pwszPtInf);

        wcsncat(pwszPtInf, NETFLT_PT_INF_REL_PATH, sizeof(NETFLT_PT_INF_REL_PATH));
        logStringW(hModule, L"vpoxNetFltQueryInfArray: INF 1: %s", pwszPtInf);

        wcsncat(pwszMpInf, NETFLT_MP_INF_REL_PATH, sizeof(NETFLT_MP_INF_REL_PATH));
        logStringW(hModule, L"vpoxNetFltQueryInfArray: INF 2: %s", pwszMpInf);
    }
    else if (uErr != ERROR_SUCCESS)
        logStringW(hModule, L"vpoxNetFltQueryInfArray: MsiGetPropertyW failed, error = 0x%x", uErr);
    else
    {
        logStringW(hModule, L"vpoxNetFltQueryInfArray: Empty installation directory");
        uErr = ERROR_GEN_FAILURE;
    }

    return uErr;
}

#endif /*VPOX_WITH_NETFLT*/

/*static*/ UINT _uninstallNetFlt(MSIHANDLE hModule)
{
#ifdef VPOX_WITH_NETFLT
    INetCfg *pNetCfg;
    UINT uErr;

    netCfgLoggerEnable(hModule);

    BOOL bOldIntMode = SetupSetNonInteractiveMode(FALSE);

    __try
    {
        logStringW(hModule, L"Uninstalling NetFlt");

        uErr = doNetCfgInit(hModule, &pNetCfg, TRUE);
        if (uErr == ERROR_SUCCESS)
        {
            HRESULT hr = VPoxNetCfgWinNetFltUninstall(pNetCfg);
            if (hr != S_OK)
                logStringW(hModule, L"UninstallNetFlt: VPoxNetCfgWinUninstallComponent failed, error = 0x%x", hr);

            uErr = errorConvertFromHResult(hModule, hr);

            VPoxNetCfgWinReleaseINetCfg(pNetCfg, TRUE);

            logStringW(hModule, L"Uninstalling NetFlt done, error = 0x%x", uErr);
        }
        else
            logStringW(hModule, L"UninstallNetFlt: doNetCfgInit failed, error = 0x%x", uErr);
    }
    __finally
    {
        if (bOldIntMode)
        {
            /* The prev mode != FALSE, i.e. non-interactive. */
            SetupSetNonInteractiveMode(bOldIntMode);
        }
        netCfgLoggerDisable();
    }
#endif /* VPOX_WITH_NETFLT */

    /* Never fail the uninstall even if we did not succeed. */
    return ERROR_SUCCESS;
}

UINT __stdcall UninstallNetFlt(MSIHANDLE hModule)
{
    (void)_uninstallNetLwf(hModule);
    return _uninstallNetFlt(hModule);
}

static UINT _installNetFlt(MSIHANDLE hModule)
{
#ifdef VPOX_WITH_NETFLT
    UINT uErr;
    INetCfg *pNetCfg;

    netCfgLoggerEnable(hModule);

    BOOL bOldIntMode = SetupSetNonInteractiveMode(FALSE);

    __try
    {

        logStringW(hModule, L"InstallNetFlt: Installing NetFlt");

        uErr = doNetCfgInit(hModule, &pNetCfg, TRUE);
        if (uErr == ERROR_SUCCESS)
        {
            WCHAR wszPtInf[MAX_PATH];
            WCHAR wszMpInf[MAX_PATH];
            uErr = vpoxNetFltQueryInfArray(hModule, wszPtInf, wszMpInf, sizeof(wszMpInf));
            if (uErr == ERROR_SUCCESS)
            {
                LPCWSTR const apwszInfs[] = { wszPtInf, wszMpInf };
                HRESULT hr = VPoxNetCfgWinNetFltInstall(pNetCfg, &apwszInfs[0], RT_ELEMENTS(apwszInfs));
                if (FAILED(hr))
                    logStringW(hModule, L"InstallNetFlt: VPoxNetCfgWinNetFltInstall failed, error = 0x%x", hr);

                uErr = errorConvertFromHResult(hModule, hr);
            }
            else
                logStringW(hModule, L"InstallNetFlt: vpoxNetFltQueryInfArray failed, error = 0x%x", uErr);

            VPoxNetCfgWinReleaseINetCfg(pNetCfg, TRUE);

            logStringW(hModule, L"InstallNetFlt: Done");
        }
        else
            logStringW(hModule, L"InstallNetFlt: doNetCfgInit failed, error = 0x%x", uErr);
    }
    __finally
    {
        if (bOldIntMode)
        {
            /* The prev mode != FALSE, i.e. non-interactive. */
            SetupSetNonInteractiveMode(bOldIntMode);
        }
        netCfgLoggerDisable();
    }
#endif /* VPOX_WITH_NETFLT */

    /* Never fail the install even if we did not succeed. */
    return ERROR_SUCCESS;
}

UINT __stdcall InstallNetFlt(MSIHANDLE hModule)
{
    (void)_uninstallNetLwf(hModule);
    return _installNetFlt(hModule);
}


/*static*/ UINT _uninstallNetLwf(MSIHANDLE hModule)
{
#ifdef VPOX_WITH_NETFLT
    INetCfg *pNetCfg;
    UINT uErr;

    netCfgLoggerEnable(hModule);

    BOOL bOldIntMode = SetupSetNonInteractiveMode(FALSE);

    __try
    {
        logStringW(hModule, L"Uninstalling NetLwf");

        uErr = doNetCfgInit(hModule, &pNetCfg, TRUE);
        if (uErr == ERROR_SUCCESS)
        {
            HRESULT hr = VPoxNetCfgWinNetLwfUninstall(pNetCfg);
            if (hr != S_OK)
                logStringW(hModule, L"UninstallNetLwf: VPoxNetCfgWinUninstallComponent failed, error = 0x%x", hr);

            uErr = errorConvertFromHResult(hModule, hr);

            VPoxNetCfgWinReleaseINetCfg(pNetCfg, TRUE);

            logStringW(hModule, L"Uninstalling NetLwf done, error = 0x%x", uErr);
        }
        else
            logStringW(hModule, L"UninstallNetLwf: doNetCfgInit failed, error = 0x%x", uErr);
    }
    __finally
    {
        if (bOldIntMode)
        {
            /* The prev mode != FALSE, i.e. non-interactive. */
            SetupSetNonInteractiveMode(bOldIntMode);
        }
        netCfgLoggerDisable();
    }
#endif /* VPOX_WITH_NETFLT */

    /* Never fail the uninstall even if we did not succeed. */
    return ERROR_SUCCESS;
}

UINT __stdcall UninstallNetLwf(MSIHANDLE hModule)
{
    (void)_uninstallNetFlt(hModule);
    return _uninstallNetLwf(hModule);
}

static UINT _installNetLwf(MSIHANDLE hModule)
{
#ifdef VPOX_WITH_NETFLT
    UINT uErr;
    INetCfg *pNetCfg;

    netCfgLoggerEnable(hModule);

    BOOL bOldIntMode = SetupSetNonInteractiveMode(FALSE);

    __try
    {

        logStringW(hModule, L"InstallNetLwf: Installing NetLwf");

        uErr = doNetCfgInit(hModule, &pNetCfg, TRUE);
        if (uErr == ERROR_SUCCESS)
        {
            WCHAR wszInf[MAX_PATH];
            DWORD cchInf = RT_ELEMENTS(wszInf) - sizeof(NETLWF_INF_NAME) - 1;
            UINT uErr = MsiGetPropertyW(hModule, L"CustomActionData", wszInf, &cchInf);
            if (uErr == ERROR_SUCCESS)
            {
                if (cchInf)
                {
                    if (wszInf[cchInf - 1] != L'\\')
                    {
                        wszInf[cchInf++] = L'\\';
                        wszInf[cchInf]   = L'\0';
                    }

                    wcscat(wszInf, NETLWF_INF_NAME);

                    HRESULT hr = VPoxNetCfgWinNetLwfInstall(pNetCfg, wszInf);
                    if (FAILED(hr))
                        logStringW(hModule, L"InstallNetLwf: VPoxNetCfgWinNetLwfInstall failed, error = 0x%x", hr);

                    uErr = errorConvertFromHResult(hModule, hr);
                }
                else
                {
                    logStringW(hModule, L"vpoxNetFltQueryInfArray: Empty installation directory");
                    uErr = ERROR_GEN_FAILURE;
                }
            }
            else
                logStringW(hModule, L"vpoxNetFltQueryInfArray: MsiGetPropertyW failed, error = 0x%x", uErr);

            VPoxNetCfgWinReleaseINetCfg(pNetCfg, TRUE);

            logStringW(hModule, L"InstallNetLwf: Done");
        }
        else
            logStringW(hModule, L"InstallNetLwf: doNetCfgInit failed, error = 0x%x", uErr);
    }
    __finally
    {
        if (bOldIntMode)
        {
            /* The prev mode != FALSE, i.e. non-interactive. */
            SetupSetNonInteractiveMode(bOldIntMode);
        }
        netCfgLoggerDisable();
    }
#endif /* VPOX_WITH_NETFLT */

    /* Never fail the install even if we did not succeed. */
    return ERROR_SUCCESS;
}

UINT __stdcall InstallNetLwf(MSIHANDLE hModule)
{
    (void)_uninstallNetFlt(hModule);
    return _installNetLwf(hModule);
}


#if 0
static BOOL RenameHostOnlyConnectionsCallback(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDev, PVOID pContext)
{
    WCHAR DevName[256];
    DWORD winEr;

    if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, pDev,
            SPDRP_FRIENDLYNAME , /* IN DWORD  Property,*/
              NULL, /*OUT PDWORD  PropertyRegDataType,  OPTIONAL*/
              (PBYTE)DevName, /*OUT PBYTE  PropertyBuffer,*/
              sizeof(DevName), /* IN DWORD  PropertyBufferSize,*/
              NULL /*OUT PDWORD  RequiredSize  OPTIONAL*/
            ))
    {
        HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, pDev,
                DICS_FLAG_GLOBAL, /* IN DWORD  Scope,*/
                0, /*IN DWORD  HwProfile, */
                DIREG_DRV, /* IN DWORD  KeyType, */
                KEY_READ /*IN REGSAM  samDesired*/
                );
        NonStandardAssert(hKey != INVALID_HANDLE_VALUE);
        if (hKey != INVALID_HANDLE_VALUE)
        {
            WCHAR guid[50];
            DWORD cbGuid=sizeof(guid);
            winEr = RegQueryValueExW(hKey,
              L"NetCfgInstanceId", /*__in_opt     LPCTSTR lpValueName,*/
              NULL, /*__reserved   LPDWORD lpReserved,*/
              NULL, /*__out_opt    LPDWORD lpType,*/
              (LPBYTE)guid, /*__out_opt    LPBYTE lpData,*/
              &cbGuid /*guid__inout_opt  LPDWORD lpcbData*/
            );
            NonStandardAssert(winEr == ERROR_SUCCESS);
            if (winEr == ERROR_SUCCESS)
            {
                WCHAR ConnectoinName[128];
                ULONG cbName = sizeof(ConnectoinName);

                HRESULT hr = VPoxNetCfgWinGenHostonlyConnectionName (DevName, ConnectoinName, &cbName);
                NonStandardAssert(hr == S_OK);
                if (SUCCEEDED(hr))
                {
                    hr = VPoxNetCfgWinRenameConnection(guid, ConnectoinName);
                    NonStandardAssert(hr == S_OK);
                }
            }
        }
        RegCloseKey(hKey);
    }
    else
    {
        NonStandardAssert(0);
    }

    return TRUE;
}
#endif

static UINT _createHostOnlyInterface(MSIHANDLE hModule, LPCWSTR pwszId, LPCWSTR pwszInfName)
{
#ifdef VPOX_WITH_NETFLT
    netCfgLoggerEnable(hModule);

    BOOL fSetupModeInteractive = SetupSetNonInteractiveMode(FALSE);

    logStringW(hModule, L"CreateHostOnlyInterface: Creating host-only interface");

    HRESULT hr = E_FAIL;
    GUID guid;
    WCHAR wszMpInf[MAX_PATH];
    DWORD cchMpInf = RT_ELEMENTS(wszMpInf) - (DWORD)wcslen(pwszInfName) - 1 - 1;
    LPCWSTR pwszInfPath = NULL;
    bool fIsFile = false;
    UINT uErr = MsiGetPropertyW(hModule, L"CustomActionData", wszMpInf, &cchMpInf);
    if (uErr == ERROR_SUCCESS)
    {
        if (cchMpInf)
        {
            logStringW(hModule, L"CreateHostOnlyInterface: NetAdpDir property = %s", wszMpInf);
            if (wszMpInf[cchMpInf - 1] != L'\\')
            {
                wszMpInf[cchMpInf++] = L'\\';
                wszMpInf[cchMpInf]   = L'\0';
            }

            wcscat(wszMpInf, pwszInfName);
            pwszInfPath = wszMpInf;
            fIsFile = true;

            logStringW(hModule, L"CreateHostOnlyInterface: Resulting INF path = %s", pwszInfPath);
        }
        else
            logStringW(hModule, L"CreateHostOnlyInterface: VPox installation path is empty");
    }
    else
        logStringW(hModule, L"CreateHostOnlyInterface: Unable to retrieve VPox installation path, error = 0x%x", uErr);

    /* Make sure the inf file is installed. */
    if (pwszInfPath != NULL && fIsFile)
    {
        logStringW(hModule, L"CreateHostOnlyInterface: Calling VPoxDrvCfgInfInstall(%s)", pwszInfPath);
        hr = VPoxDrvCfgInfInstall(pwszInfPath);
        logStringW(hModule, L"CreateHostOnlyInterface: VPoxDrvCfgInfInstall returns 0x%x", hr);
        if (FAILED(hr))
            logStringW(hModule, L"CreateHostOnlyInterface: Failed to install INF file, error = 0x%x", hr);
    }

    if (SUCCEEDED(hr))
    {
        //first, try to update Host Only Network Interface
        BOOL fRebootRequired = FALSE;
        hr = VPoxNetCfgWinUpdateHostOnlyNetworkInterface(pwszInfPath, &fRebootRequired, pwszId);
        if (SUCCEEDED(hr))
        {
            if (fRebootRequired)
            {
                logStringW(hModule, L"CreateHostOnlyInterface: Reboot required for update, setting REBOOT property to force");
                HRESULT hr2 = MsiSetPropertyW(hModule, L"REBOOT", L"Force");
                if (hr2 != ERROR_SUCCESS)
                    logStringW(hModule, L"CreateHostOnlyInterface: Failed to set REBOOT property for update, error = 0x%x", hr2);
            }
        }
        else
        {
            //in fail case call CreateHostOnlyInterface
            logStringW(hModule, L"CreateHostOnlyInterface: VPoxNetCfgWinUpdateHostOnlyNetworkInterface failed, hr = 0x%x", hr);
            logStringW(hModule, L"CreateHostOnlyInterface: calling VPoxNetCfgWinCreateHostOnlyNetworkInterface");
#ifdef VPOXNETCFG_DELAYEDRENAME
            BSTR devId;
            hr = VPoxNetCfgWinCreateHostOnlyNetworkInterface(pwszInfPath, fIsFile, NULL, &guid, &devId, NULL);
#else /* !VPOXNETCFG_DELAYEDRENAME */
            hr = VPoxNetCfgWinCreateHostOnlyNetworkInterface(pwszInfPath, fIsFile, NULL, &guid, NULL, NULL);
#endif /* !VPOXNETCFG_DELAYEDRENAME */
            logStringW(hModule, L"CreateHostOnlyInterface: VPoxNetCfgWinCreateHostOnlyNetworkInterface returns 0x%x", hr);
            if (SUCCEEDED(hr))
            {
                ULONG ip = inet_addr("192.168.56.1");
                ULONG mask = inet_addr("255.255.255.0");
                logStringW(hModule, L"CreateHostOnlyInterface: calling VPoxNetCfgWinEnableStaticIpConfig");
                hr = VPoxNetCfgWinEnableStaticIpConfig(&guid, ip, mask);
                logStringW(hModule, L"CreateHostOnlyInterface: VPoxNetCfgWinEnableStaticIpConfig returns 0x%x", hr);
                if (FAILED(hr))
                    logStringW(hModule, L"CreateHostOnlyInterface: VPoxNetCfgWinEnableStaticIpConfig failed, error = 0x%x", hr);
#ifdef VPOXNETCFG_DELAYEDRENAME
                hr = VPoxNetCfgWinRenameHostOnlyConnection(&guid, devId, NULL);
                if (FAILED(hr))
                    logStringW(hModule, L"CreateHostOnlyInterface: VPoxNetCfgWinRenameHostOnlyConnection failed, error = 0x%x", hr);
                SysFreeString(devId);
#endif /* VPOXNETCFG_DELAYEDRENAME */
            }
            else
                logStringW(hModule, L"CreateHostOnlyInterface: VPoxNetCfgWinCreateHostOnlyNetworkInterface failed, error = 0x%x", hr);
        }
    }

    if (SUCCEEDED(hr))
        logStringW(hModule, L"CreateHostOnlyInterface: Creating host-only interface done");

    /* Restore original setup mode. */
    logStringW(hModule, L"CreateHostOnlyInterface: Almost done...");
    if (fSetupModeInteractive)
        SetupSetNonInteractiveMode(fSetupModeInteractive);

    netCfgLoggerDisable();

#endif /* VPOX_WITH_NETFLT */

    logStringW(hModule, L"CreateHostOnlyInterface: Returns success (ignoring all failures)");
    /* Never fail the install even if we did not succeed. */
    return ERROR_SUCCESS;
}

UINT __stdcall CreateHostOnlyInterface(MSIHANDLE hModule)
{
    return _createHostOnlyInterface(hModule, NETADP_ID, L"VPoxNetAdp.inf");
}

UINT __stdcall Ndis6CreateHostOnlyInterface(MSIHANDLE hModule)
{
    return _createHostOnlyInterface(hModule, NETADP_ID, L"VPoxNetAdp6.inf");
}

static UINT _removeHostOnlyInterfaces(MSIHANDLE hModule, LPCWSTR pwszId)
{
#ifdef VPOX_WITH_NETFLT
    netCfgLoggerEnable(hModule);

    logStringW(hModule, L"RemoveHostOnlyInterfaces: Removing all host-only interfaces");

    BOOL fSetupModeInteractive = SetupSetNonInteractiveMode(FALSE);

    HRESULT hr = VPoxNetCfgWinRemoveAllNetDevicesOfId(pwszId);
    if (SUCCEEDED(hr))
    {
        hr = VPoxDrvCfgInfUninstallAllSetupDi(&GUID_DEVCLASS_NET, L"Net", pwszId, SUOI_FORCEDELETE/* could be SUOI_FORCEDELETE */);
        if (FAILED(hr))
        {
            logStringW(hModule, L"RemoveHostOnlyInterfaces: NetAdp uninstalled successfully, but failed to remove INF files");
        }
        else
            logStringW(hModule, L"RemoveHostOnlyInterfaces: NetAdp uninstalled successfully");

    }
    else
        logStringW(hModule, L"RemoveHostOnlyInterfaces: NetAdp uninstall failed, hr = 0x%x", hr);

    /* Restore original setup mode. */
    if (fSetupModeInteractive)
        SetupSetNonInteractiveMode(fSetupModeInteractive);

    netCfgLoggerDisable();
#endif /* VPOX_WITH_NETFLT */

    /* Never fail the uninstall even if we did not succeed. */
    return ERROR_SUCCESS;
}

UINT __stdcall RemoveHostOnlyInterfaces(MSIHANDLE hModule)
{
    return _removeHostOnlyInterfaces(hModule, NETADP_ID);
}

static UINT _stopHostOnlyInterfaces(MSIHANDLE hModule, LPCWSTR pwszId)
{
#ifdef VPOX_WITH_NETFLT
    netCfgLoggerEnable(hModule);

    logStringW(hModule, L"StopHostOnlyInterfaces: Stopping all host-only interfaces");

    BOOL fSetupModeInteractive = SetupSetNonInteractiveMode(FALSE);

    HRESULT hr = VPoxNetCfgWinPropChangeAllNetDevicesOfId(pwszId, VPOXNECTFGWINPROPCHANGE_TYPE_DISABLE);
    if (SUCCEEDED(hr))
    {
        logStringW(hModule, L"StopHostOnlyInterfaces: Disabling host interfaces was successful, hr = 0x%x", hr);
    }
    else
        logStringW(hModule, L"StopHostOnlyInterfaces: Disabling host interfaces failed, hr = 0x%x", hr);

    /* Restore original setup mode. */
    if (fSetupModeInteractive)
        SetupSetNonInteractiveMode(fSetupModeInteractive);

    netCfgLoggerDisable();
#endif /* VPOX_WITH_NETFLT */

    /* Never fail the uninstall even if we did not succeed. */
    return ERROR_SUCCESS;
}

UINT __stdcall StopHostOnlyInterfaces(MSIHANDLE hModule)
{
    return _stopHostOnlyInterfaces(hModule, NETADP_ID);
}

static UINT _updateHostOnlyInterfaces(MSIHANDLE hModule, LPCWSTR pwszInfName, LPCWSTR pwszId)
{
#ifdef VPOX_WITH_NETFLT
    netCfgLoggerEnable(hModule);

    logStringW(hModule, L"UpdateHostOnlyInterfaces: Updating all host-only interfaces");

    BOOL fSetupModeInteractive = SetupSetNonInteractiveMode(FALSE);

    WCHAR wszMpInf[MAX_PATH];
    DWORD cchMpInf = RT_ELEMENTS(wszMpInf) - (DWORD)wcslen(pwszInfName) - 1 - 1;
    LPCWSTR pwszInfPath = NULL;
    bool fIsFile = false;
    UINT uErr = MsiGetPropertyW(hModule, L"CustomActionData", wszMpInf, &cchMpInf);
    if (uErr == ERROR_SUCCESS)
    {
        if (cchMpInf)
        {
            logStringW(hModule, L"UpdateHostOnlyInterfaces: NetAdpDir property = %s", wszMpInf);
            if (wszMpInf[cchMpInf - 1] != L'\\')
            {
                wszMpInf[cchMpInf++] = L'\\';
                wszMpInf[cchMpInf]   = L'\0';
            }

            wcscat(wszMpInf, pwszInfName);
            pwszInfPath = wszMpInf;
            fIsFile = true;

            logStringW(hModule, L"UpdateHostOnlyInterfaces: Resulting INF path = %s", pwszInfPath);

            DWORD attrFile = GetFileAttributesW(pwszInfPath);
            if (attrFile == INVALID_FILE_ATTRIBUTES)
            {
                DWORD dwErr = GetLastError();
                logStringW(hModule, L"UpdateHostOnlyInterfaces: File \"%s\" not found, dwErr=%ld",
                           pwszInfPath, dwErr);
            }
            else
            {
                logStringW(hModule, L"UpdateHostOnlyInterfaces: File \"%s\" exists",
                           pwszInfPath);

                BOOL fRebootRequired = FALSE;
                HRESULT hr = VPoxNetCfgWinUpdateHostOnlyNetworkInterface(pwszInfPath, &fRebootRequired, pwszId);
                if (SUCCEEDED(hr))
                {
                    if (fRebootRequired)
                    {
                        logStringW(hModule, L"UpdateHostOnlyInterfaces: Reboot required, setting REBOOT property to force");
                        HRESULT hr2 = MsiSetPropertyW(hModule, L"REBOOT", L"Force");
                        if (hr2 != ERROR_SUCCESS)
                            logStringW(hModule, L"UpdateHostOnlyInterfaces: Failed to set REBOOT property, error = 0x%x", hr2);
                    }
                }
                else
                    logStringW(hModule, L"UpdateHostOnlyInterfaces: VPoxNetCfgWinUpdateHostOnlyNetworkInterface failed, hr = 0x%x", hr);
            }
        }
        else
            logStringW(hModule, L"UpdateHostOnlyInterfaces: VPox installation path is empty");
    }
    else
        logStringW(hModule, L"UpdateHostOnlyInterfaces: Unable to retrieve VPox installation path, error = 0x%x", uErr);

    /* Restore original setup mode. */
    if (fSetupModeInteractive)
        SetupSetNonInteractiveMode(fSetupModeInteractive);

    netCfgLoggerDisable();
#endif /* VPOX_WITH_NETFLT */

    /* Never fail the update even if we did not succeed. */
    return ERROR_SUCCESS;
}

UINT __stdcall UpdateHostOnlyInterfaces(MSIHANDLE hModule)
{
    return _updateHostOnlyInterfaces(hModule, L"VPoxNetAdp.inf", NETADP_ID);
}

UINT __stdcall Ndis6UpdateHostOnlyInterfaces(MSIHANDLE hModule)
{
    return _updateHostOnlyInterfaces(hModule, L"VPoxNetAdp6.inf", NETADP_ID);
}

static UINT _uninstallNetAdp(MSIHANDLE hModule, LPCWSTR pwszId)
{
#ifdef VPOX_WITH_NETFLT
    INetCfg *pNetCfg;
    UINT uErr;

    netCfgLoggerEnable(hModule);

    BOOL bOldIntMode = SetupSetNonInteractiveMode(FALSE);

    __try
    {
        logStringW(hModule, L"Uninstalling NetAdp");

        uErr = doNetCfgInit(hModule, &pNetCfg, TRUE);
        if (uErr == ERROR_SUCCESS)
        {
            HRESULT hr = VPoxNetCfgWinNetAdpUninstall(pNetCfg, pwszId);
            if (hr != S_OK)
                logStringW(hModule, L"UninstallNetAdp: VPoxNetCfgWinUninstallComponent failed, error = 0x%x", hr);

            uErr = errorConvertFromHResult(hModule, hr);

            VPoxNetCfgWinReleaseINetCfg(pNetCfg, TRUE);

            logStringW(hModule, L"Uninstalling NetAdp done, error = 0x%x", uErr);
        }
        else
            logStringW(hModule, L"UninstallNetAdp: doNetCfgInit failed, error = 0x%x", uErr);
    }
    __finally
    {
        if (bOldIntMode)
        {
            /* The prev mode != FALSE, i.e. non-interactive. */
            SetupSetNonInteractiveMode(bOldIntMode);
        }
        netCfgLoggerDisable();
    }
#endif /* VPOX_WITH_NETFLT */

    /* Never fail the uninstall even if we did not succeed. */
    return ERROR_SUCCESS;
}

UINT __stdcall UninstallNetAdp(MSIHANDLE hModule)
{
    return _uninstallNetAdp(hModule, NETADP_ID);
}

static bool isTAPDevice(const WCHAR *pwszGUID)
{
    HKEY hNetcard;
    bool bIsTapDevice = false;
    LONG lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                 L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}",
                                 0, KEY_READ, &hNetcard);
    if (lStatus != ERROR_SUCCESS)
        return false;

    int i = 0;
    for (;;)
    {
        WCHAR wszEnumName[256];
        WCHAR wszNetCfgInstanceId[256];
        DWORD dwKeyType;
        HKEY  hNetCardGUID;

        DWORD dwLen = sizeof(wszEnumName);
        lStatus = RegEnumKeyExW(hNetcard, i, wszEnumName, &dwLen, NULL, NULL, NULL, NULL);
        if (lStatus != ERROR_SUCCESS)
            break;

        lStatus = RegOpenKeyExW(hNetcard, wszEnumName, 0, KEY_READ, &hNetCardGUID);
        if (lStatus == ERROR_SUCCESS)
        {
            dwLen = sizeof(wszNetCfgInstanceId);
            lStatus = RegQueryValueExW(hNetCardGUID, L"NetCfgInstanceId", NULL, &dwKeyType, (LPBYTE)wszNetCfgInstanceId, &dwLen);
            if (   lStatus == ERROR_SUCCESS
                && dwKeyType == REG_SZ)
            {
                WCHAR wszNetProductName[256];
                WCHAR wszNetProviderName[256];

                wszNetProductName[0] = 0;
                dwLen = sizeof(wszNetProductName);
                lStatus = RegQueryValueExW(hNetCardGUID, L"ProductName", NULL, &dwKeyType, (LPBYTE)wszNetProductName, &dwLen);

                wszNetProviderName[0] = 0;
                dwLen = sizeof(wszNetProviderName);
                lStatus = RegQueryValueExW(hNetCardGUID, L"ProviderName", NULL, &dwKeyType, (LPBYTE)wszNetProviderName, &dwLen);

                if (   !wcscmp(wszNetCfgInstanceId, pwszGUID)
                    && !wcscmp(wszNetProductName, L"VirtualPox TAP Adapter")
                    && (   (!wcscmp(wszNetProviderName, L"immotek GmbH")) /* Legacy stuff. */
                        || (!wcscmp(wszNetProviderName, L"Sun Microsystems, Inc.")) /* Legacy stuff. */
                        || (!wcscmp(wszNetProviderName, MY_WTEXT(VPOX_VENDOR))) /* Reflects current vendor string. */
                       )
                   )
                {
                    bIsTapDevice = true;
                    RegCloseKey(hNetCardGUID);
                    break;
                }
            }
            RegCloseKey(hNetCardGUID);
        }
        ++i;
    }

    RegCloseKey(hNetcard);
    return bIsTapDevice;
}

#define SetErrBreak(args) \
    if (1) { \
        rc = 0; \
        logStringW args; \
        break; \
    } else do {} while (0)

int removeNetworkInterface(MSIHANDLE hModule, const WCHAR *pwszGUID)
{
    int rc = 1;
    do
    {
        WCHAR wszPnPInstanceId[512] = {0};

        /* We have to find the device instance ID through a registry search */

        HKEY hkeyNetwork = 0;
        HKEY hkeyConnection = 0;

        do /* break-loop */
        {
            WCHAR wszRegLocation[256];
            swprintf(wszRegLocation,
                     L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%s",
                     pwszGUID);
            LONG lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszRegLocation, 0, KEY_READ, &hkeyNetwork);
            if ((lStatus != ERROR_SUCCESS) || !hkeyNetwork)
                SetErrBreak((hModule, L"VPox HostInterfaces: Host interface network was not found in registry (%s)! [1]",
                             wszRegLocation));

            lStatus = RegOpenKeyExW(hkeyNetwork, L"Connection", 0, KEY_READ, &hkeyConnection);
            if ((lStatus != ERROR_SUCCESS) || !hkeyConnection)
                SetErrBreak((hModule, L"VPox HostInterfaces: Host interface network was not found in registry (%s)! [2]",
                             wszRegLocation));

            DWORD len = sizeof(wszPnPInstanceId);
            DWORD dwKeyType;
            lStatus = RegQueryValueExW(hkeyConnection, L"PnPInstanceID", NULL,
                                       &dwKeyType, (LPBYTE)&wszPnPInstanceId[0], &len);
            if ((lStatus != ERROR_SUCCESS) || (dwKeyType != REG_SZ))
                SetErrBreak((hModule, L"VPox HostInterfaces: Host interface network was not found in registry (%s)! [3]",
                             wszRegLocation));
        }
        while (0);

        if (hkeyConnection)
            RegCloseKey(hkeyConnection);
        if (hkeyNetwork)
            RegCloseKey(hkeyNetwork);

        /*
         * Now we are going to enumerate all network devices and
         * wait until we encounter the right device instance ID
         */

        HDEVINFO hDeviceInfo = INVALID_HANDLE_VALUE;
        BOOL fResult;

        do
        {
            GUID netGuid;
            SP_DEVINFO_DATA DeviceInfoData;
            DWORD index = 0;
            DWORD size = 0;

            /* initialize the structure size */
            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            /* copy the net class GUID */
            memcpy(&netGuid, &GUID_DEVCLASS_NET, sizeof (GUID_DEVCLASS_NET));

            /* return a device info set contains all installed devices of the Net class */
            hDeviceInfo = SetupDiGetClassDevs(&netGuid, NULL, NULL, DIGCF_PRESENT);
            if (hDeviceInfo == INVALID_HANDLE_VALUE)
            {
                logStringW(hModule, L"VPox HostInterfaces: SetupDiGetClassDevs failed (0x%08X)!", GetLastError());
                SetErrBreak((hModule, L"VPox HostInterfaces: Uninstallation failed!"));
            }

            BOOL fFoundDevice = FALSE;

            /* enumerate the driver info list */
            while (TRUE)
            {
                WCHAR *pwszDeviceHwid;

                fResult = SetupDiEnumDeviceInfo(hDeviceInfo, index, &DeviceInfoData);
                if (!fResult)
                {
                    if (GetLastError() == ERROR_NO_MORE_ITEMS)
                        break;
                    else
                    {
                        index++;
                        continue;
                    }
                }

                /* try to get the hardware ID registry property */
                fResult = SetupDiGetDeviceRegistryProperty(hDeviceInfo,
                                                           &DeviceInfoData,
                                                           SPDRP_HARDWAREID,
                                                           NULL,
                                                           NULL,
                                                           0,
                                                           &size);
                if (!fResult)
                {
                    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    {
                        index++;
                        continue;
                    }

                    pwszDeviceHwid = (WCHAR *)malloc(size);
                    if (pwszDeviceHwid)
                    {
                        fResult = SetupDiGetDeviceRegistryProperty(hDeviceInfo,
                                                                   &DeviceInfoData,
                                                                   SPDRP_HARDWAREID,
                                                                   NULL,
                                                                   (PBYTE)pwszDeviceHwid,
                                                                   size,
                                                                   NULL);
                        if (!fResult)
                        {
                            free(pwszDeviceHwid);
                            pwszDeviceHwid = NULL;
                            index++;
                            continue;
                        }
                    }
                }
                else
                {
                    /* something is wrong.  This shouldn't have worked with a NULL buffer */
                    index++;
                    continue;
                }

                for (WCHAR *t = pwszDeviceHwid;
                     t && *t && t < &pwszDeviceHwid[size / sizeof(WCHAR)];
                     t += wcslen(t) + 1)
                {
                    if (!_wcsicmp(L"vpoxtap", t))
                    {
                          /* get the device instance ID */
                          WCHAR wszDevID[MAX_DEVICE_ID_LEN];
                          if (CM_Get_Device_IDW(DeviceInfoData.DevInst,
                                                wszDevID, MAX_DEVICE_ID_LEN, 0) == CR_SUCCESS)
                          {
                              /* compare to what we determined before */
                              if (!wcscmp(wszDevID, wszPnPInstanceId))
                              {
                                  fFoundDevice = TRUE;
                                  break;
                              }
                          }
                    }
                }

                if (pwszDeviceHwid)
                {
                    free(pwszDeviceHwid);
                    pwszDeviceHwid = NULL;
                }

                if (fFoundDevice)
                    break;

                index++;
            }

            if (fFoundDevice)
            {
                fResult = SetupDiSetSelectedDevice(hDeviceInfo, &DeviceInfoData);
                if (!fResult)
                {
                    logStringW(hModule, L"VPox HostInterfaces: SetupDiSetSelectedDevice failed (0x%08X)!", GetLastError());
                    SetErrBreak((hModule, L"VPox HostInterfaces: Uninstallation failed!"));
                }

                fResult = SetupDiCallClassInstaller(DIF_REMOVE, hDeviceInfo, &DeviceInfoData);
                if (!fResult)
                {
                    logStringW(hModule, L"VPox HostInterfaces: SetupDiCallClassInstaller (DIF_REMOVE) failed (0x%08X)!", GetLastError());
                    SetErrBreak((hModule, L"VPox HostInterfaces: Uninstallation failed!"));
                }
            }
            else
                SetErrBreak((hModule, L"VPox HostInterfaces: Host interface network device not found!"));
        } while (0);

        /* clean up the device info set */
        if (hDeviceInfo != INVALID_HANDLE_VALUE)
            SetupDiDestroyDeviceInfoList(hDeviceInfo);
    } while (0);
    return rc;
}

UINT __stdcall UninstallTAPInstances(MSIHANDLE hModule)
{
    static const WCHAR *s_wszNetworkKey = L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
    HKEY hCtrlNet;

    LONG lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, s_wszNetworkKey, 0, KEY_READ, &hCtrlNet);
    if (lStatus == ERROR_SUCCESS)
    {
        logStringW(hModule, L"VPox HostInterfaces: Enumerating interfaces ...");
        for (int i = 0; ; ++i)
        {
            WCHAR wszNetworkGUID[256] = { 0 };
            DWORD dwLen = (DWORD)sizeof(wszNetworkGUID);
            lStatus = RegEnumKeyExW(hCtrlNet, i, wszNetworkGUID, &dwLen, NULL, NULL, NULL, NULL);
            if (lStatus != ERROR_SUCCESS)
            {
                switch (lStatus)
                {
                    case ERROR_NO_MORE_ITEMS:
                        logStringW(hModule, L"VPox HostInterfaces: No interfaces found.");
                        break;
                    default:
                        logStringW(hModule, L"VPox HostInterfaces: Enumeration failed: %ld", lStatus);
                        break;
                }
                break;
            }

            if (isTAPDevice(wszNetworkGUID))
            {
                logStringW(hModule, L"VPox HostInterfaces: Removing interface \"%s\" ...", wszNetworkGUID);
                removeNetworkInterface(hModule, wszNetworkGUID);
                lStatus = RegDeleteKeyW(hCtrlNet, wszNetworkGUID);
            }
        }
        RegCloseKey(hCtrlNet);
        logStringW(hModule, L"VPox HostInterfaces: Removing interfaces done.");
    }
    return ERROR_SUCCESS;
}


/**
 * This is used to remove the old VPoxDrv service before installation.
 *
 * The current service name is VPoxSup but the INF file won't remove the old
 * one, so we do it manually to try prevent trouble as the device nodes are the
 * same and we would fail starting VPoxSup.sys if VPoxDrv.sys is still loading.
 *
 * Status code is ignored for now as a reboot should fix most potential trouble
 * here (and I don't want to break stuff too badly).
 *
 * @sa @bugref{10162}
 */
UINT __stdcall UninstallVPoxDrv(MSIHANDLE hModule)
{
    /*
     * Try open the service.
     */
    SC_HANDLE hSMgr = OpenSCManager(NULL, NULL, SERVICE_CHANGE_CONFIG | SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (hSMgr)
    {
        SC_HANDLE hService = OpenServiceW(hSMgr, L"VPoxDrv", DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (hService)
        {
            /*
             * Try stop it before we delete it.
             */
            SERVICE_STATUS Status = { 0, 0, 0, 0, 0, 0, 0 };
            QueryServiceStatus(hService, &Status);
            if (Status.dwCurrentState == SERVICE_STOPPED)
                logString(hModule, "VPoxDrv: The service old service was already stopped");
            else
            {
                logString(hModule, "VPoxDrv: Stopping the service (state %u)", Status.dwCurrentState);
                if (ControlService(hService, SERVICE_CONTROL_STOP, &Status))
                {
                    /* waiting for it to stop: */
                    int iWait = 100;
                    while (Status.dwCurrentState == SERVICE_STOP_PENDING && iWait-- > 0)
                    {
                        Sleep(100);
                        QueryServiceStatus(hService, &Status);
                    }

                    if (Status.dwCurrentState == SERVICE_STOPPED)
                        logString(hModule, "VPoxDrv: Stopped service");
                    else
                        logString(hModule, "VPoxDrv: Failed to stop the service, status: %u", Status.dwCurrentState);
                }
                else
                {
                    DWORD const dwErr = GetLastError();
                    if (   Status.dwCurrentState == SERVICE_STOP_PENDING
                        && dwErr == ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
                        logString(hModule, "VPoxDrv: Failed to stop the service: stop pending, not accepting control messages");
                    else
                        logString(hModule, "VPoxDrv: Failed to stop the service: dwErr=%u status=%u", dwErr, Status.dwCurrentState);
                }
            }

            /*
             * Delete the service, or at least mark it for deletion.
             */
            if (DeleteService(hService))
                logString(hModule, "VPoxDrv: Successfully delete service");
            else
                logString(hModule, "VPoxDrv: Failed to delete the service: %u", GetLastError());

            CloseServiceHandle(hService);
        }
        else
        {
            DWORD const dwErr = GetLastError();
            if (dwErr == ERROR_SERVICE_DOES_NOT_EXIST)
                logString(hModule, "VPoxDrv: Nothing to do, the old service does not exist");
            else
                logString(hModule, "VPoxDrv: Failed to open the service: %u", dwErr);
        }

        CloseServiceHandle(hSMgr);
    }
    else
        logString(hModule, "VPoxDrv: Failed to open service manager (%u).", GetLastError());

    return ERROR_SUCCESS;
}

