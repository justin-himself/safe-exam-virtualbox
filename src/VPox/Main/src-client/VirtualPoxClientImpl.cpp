/* $Id: VirtualPoxClientImpl.cpp $ */
/** @file
 * VirtualPox COM class implementation
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

#define LOG_GROUP LOG_GROUP_MAIN_VIRTUALPOXCLIENT
#include "LoggingNew.h"

#include "VirtualPoxClientImpl.h"

#include "AutoCaller.h"
#include "VPoxEvents.h"
#include "VPox/com/ErrorInfo.h"

#include <iprt/asm.h>
#include <iprt/thread.h>
#include <iprt/critsect.h>
#include <iprt/semaphore.h>
#include <iprt/cpp/utils.h>
#include <iprt/utf16.h>
#ifdef RT_OS_WINDOWS
# include <iprt/err.h>
# include <iprt/ldr.h>
# include <msi.h>
# include <WbemIdl.h>
#endif


/** Waiting time between probing whether VPoxSVC is alive. */
#define VPOXCLIENT_DEFAULT_INTERVAL 30000


/** Initialize instance counter class variable */
uint32_t VirtualPoxClient::g_cInstances = 0;

LONG VirtualPoxClient::s_cUnnecessaryAtlModuleLocks = 0;

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

HRESULT VirtualPoxClient::FinalConstruct()
{
    HRESULT rc = init();
    BaseFinalConstruct();
    return rc;
}

void VirtualPoxClient::FinalRelease()
{
    uninit();
    BaseFinalRelease();
}


// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the VirtualPoxClient object.
 *
 * @returns COM result indicator
 */
HRESULT VirtualPoxClient::init()
{

#if defined(RT_OS_WINDOWS) && defined(VPOX_WITH_SDS)
    // setup COM Security to enable impersonation
    // This works for console VirtualPox clients, GUI has own security settings
    //  For GUI VirtualPox it will be second call so can return TOO_LATE error
    HRESULT hrGUICoInitializeSecurity = CoInitializeSecurity(NULL,
                                                             -1,
                                                             NULL,
                                                             NULL,
                                                             RPC_C_AUTHN_LEVEL_DEFAULT,
                                                             RPC_C_IMP_LEVEL_IMPERSONATE,
                                                             NULL,
                                                             EOAC_NONE,
                                                             NULL);
    NOREF(hrGUICoInitializeSecurity);
    Assert(SUCCEEDED(hrGUICoInitializeSecurity) || hrGUICoInitializeSecurity == RPC_E_TOO_LATE);
#endif

    LogFlowThisFuncEnter();

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    /* Important: DO NOT USE any kind of "early return" (except the single
     * one above, checking the init span success) in this method. It is vital
     * for correct error handling that it has only one point of return, which
     * does all the magic on COM to signal object creation success and
     * reporting the error later for every API method. COM translates any
     * unsuccessful object creation to REGDB_E_CLASSNOTREG errors or similar
     * unhelpful ones which cause us a lot of grief with troubleshooting. */

    HRESULT rc = S_OK;
    try
    {
        if (ASMAtomicIncU32(&g_cInstances) != 1)
            AssertFailedStmt(throw setError(E_FAIL, tr("Attempted to create more than one VirtualPoxClient instance")));

        mData.m_ThreadWatcher = NIL_RTTHREAD;
        mData.m_SemEvWatcher = NIL_RTSEMEVENT;

        rc = mData.m_pVirtualPox.createLocalObject(CLSID_VirtualPox);
        if (FAILED(rc))
#ifdef RT_OS_WINDOWS
            throw i_investigateVirtualPoxObjectCreationFailure(rc);
#else
            throw rc;
#endif

        /* VirtualPox error return is postponed to method calls, fetch it. */
        ULONG rev;
        rc = mData.m_pVirtualPox->COMGETTER(Revision)(&rev);
        if (FAILED(rc))
            throw rc;

        rc = unconst(mData.m_pEventSource).createObject();
        AssertComRCThrow(rc, setError(rc, tr("Could not create EventSource for VirtualPoxClient")));
        rc = mData.m_pEventSource->init();
        AssertComRCThrow(rc, setError(rc, tr("Could not initialize EventSource for VirtualPoxClient")));

        /* HACK ALERT! This is for DllCanUnloadNow(). */
        s_cUnnecessaryAtlModuleLocks++;
        AssertMsg(s_cUnnecessaryAtlModuleLocks == 1, ("%d\n", s_cUnnecessaryAtlModuleLocks));

        /* Setting up the VPoxSVC watcher thread. If anything goes wrong here it
         * is not considered important enough to cause any sort of visible
         * failure. The monitoring will not be done, but that's all. */
        int vrc = RTSemEventCreate(&mData.m_SemEvWatcher);
        if (RT_FAILURE(vrc))
        {
            mData.m_SemEvWatcher = NIL_RTSEMEVENT;
            AssertRCStmt(vrc, throw setErrorBoth(VPOX_E_IPRT_ERROR, vrc, tr("Failed to create semaphore (rc=%Rrc)"), vrc));
        }

        vrc = RTThreadCreate(&mData.m_ThreadWatcher, SVCWatcherThread, this, 0,
                             RTTHREADTYPE_INFREQUENT_POLLER, RTTHREADFLAGS_WAITABLE, "VPoxSVCWatcher");
        if (RT_FAILURE(vrc))
        {
            RTSemEventDestroy(mData.m_SemEvWatcher);
            mData.m_SemEvWatcher = NIL_RTSEMEVENT;
            AssertRCStmt(vrc, throw setErrorBoth(VPOX_E_IPRT_ERROR, vrc,  tr("Failed to create watcher thread (rc=%Rrc)"), vrc));
        }
    }
    catch (HRESULT err)
    {
        /* we assume that error info is set by the thrower */
        rc = err;
    }
    catch (...)
    {
        rc = VirtualPoxBase::handleUnexpectedExceptions(this, RT_SRC_POS);
    }

    /* Confirm a successful initialization when it's the case. Must be last,
     * as on failure it will uninitialize the object. */
    if (SUCCEEDED(rc))
        autoInitSpan.setSucceeded();
    else
        autoInitSpan.setFailed(rc);

    LogFlowThisFunc(("rc=%Rhrc\n", rc));
    LogFlowThisFuncLeave();
    /* Unconditionally return success, because the error return is delayed to
     * the attribute/method calls through the InitFailed object state. */
    return S_OK;
}

#ifdef RT_OS_WINDOWS

/**
 * Looks into why we failed to create the VirtualPox object.
 *
 * @returns hrcCaller thru setError.
 * @param   hrcCaller   The failure status code.
 */
HRESULT VirtualPoxClient::i_investigateVirtualPoxObjectCreationFailure(HRESULT hrcCaller)
{
    HRESULT hrc;

# ifdef VPOX_WITH_SDS
    /*
     * Check that the VPoxSDS service is configured to run as LocalSystem and is enabled.
     */
    WCHAR    wszBuffer[256];
    uint32_t uStartType;
    int vrc = i_getServiceAccountAndStartType(L"VPoxSDS", wszBuffer, RT_ELEMENTS(wszBuffer), &uStartType);
    if (RT_SUCCESS(vrc))
    {
        LogRelFunc(("VPoxSDS service is running under the '%ls' account with start type %u.\n", wszBuffer, uStartType));
        if (RTUtf16Cmp(wszBuffer, L"LocalSystem") != 0)
            return setError(hrcCaller,
                            tr("VPoxSDS is misconfigured to run under the '%ls' account instead of the SYSTEM one.\n"
                               "Reinstall VirtualPox to fix it.  Alternatively you can fix it using the Windows Service Control "
                               "Manager or by running 'qc config VPoxSDS obj=LocalSystem' on a command line."), wszBuffer);
        if (uStartType == SERVICE_DISABLED)
            return setError(hrcCaller,
                            tr("The VPoxSDS windows service is disabled.\n"
                               "Reinstall VirtualPox to fix it.  Alternatively try reenable the service by setting it to "
                               " 'Manual' startup type in the Windows Service management console, or by runing "
                               "'sc config VPoxSDS start=demand' on the command line."));
    }
    else if (vrc == VERR_NOT_FOUND)
        return setError(hrcCaller,
                        tr("The VPoxSDS windows service was not found.\n"
                           "Reinstall VirtualPox to fix it.  Alternatively you can try start VirtualPox as Administrator, this "
                           "should automatically reinstall the service, or you can run "
                           "'VPoxSDS.exe --regservice' command from an elevated Administrator command line."));
    else
        LogRelFunc(("VirtualPoxClient::i_getServiceAccount failed: %Rrc\n", vrc));
# endif

    /*
     * First step is to try get an IUnknown interface of the VirtualPox object.
     *
     * This will succeed even when oleaut32.msm (see @bugref{8016}, @ticketref{12087})
     * is accidentally installed and messes up COM.  It may also succeed when the COM
     * registration is partially broken (though that's unlikely to happen these days).
     */
    IUnknown *pUnknown = NULL;
    hrc = CoCreateInstance(CLSID_VirtualPox, NULL, CLSCTX_LOCAL_SERVER, IID_IUnknown, (void **)&pUnknown);
    if (FAILED(hrc))
    {
        if (hrc == hrcCaller)
            return setError(hrcCaller, tr("Completely failed to instantiate CLSID_VirtualPox: %Rhrc"), hrcCaller);
        return setError(hrcCaller, tr("Completely failed to instantiate CLSID_VirtualPox: %Rhrc & %Rhrc"), hrcCaller, hrc);
    }

    /*
     * Try query the IVirtualPox interface (should fail), if it succeed we return
     * straight away so we have more columns to spend on long messages below.
     */
    IVirtualPox *pVirtualPox;
    hrc = pUnknown->QueryInterface(IID_IVirtualPox, (void **)&pVirtualPox);
    if (SUCCEEDED(hrc))
    {
        pVirtualPox->Release();
        pUnknown->Release();
        return setError(hrcCaller,
                        tr("Failed to instantiate CLSID_VirtualPox the first time, but worked when checking out why ... weird"));
    }

    /*
     * Check for oleaut32.msm traces in the registry.
     */
    HKEY hKey;
    LSTATUS lrc = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID\\{00020420-0000-0000-C000-000000000046}\\InprocServer32",
                                0 /*fFlags*/, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | STANDARD_RIGHTS_READ, &hKey);
    if (lrc == ERROR_SUCCESS)
    {
        wchar_t wszBuf[8192];
        DWORD   cbBuf  = sizeof(wszBuf) - sizeof(wchar_t);
        DWORD   dwType = 0;
        lrc = RegQueryValueExW(hKey, L"InprocServer32", NULL /*pvReserved*/, &dwType, (BYTE *)&wszBuf[0], &cbBuf);
        if (lrc == ERROR_SUCCESS)
        {
            wszBuf[cbBuf / sizeof(wchar_t)] = '\0';
            bool fSetError = false;

            /*
             * Try decode the string and improve the message.
             */
            typedef UINT (WINAPI *PFNMSIDECOMPOSEDESCRIPTORW)(PCWSTR pwszDescriptor,
                                                              LPWSTR pwszProductCode /*[40]*/,
                                                              LPWSTR pwszFeatureId /*[40]*/,
                                                              LPWSTR pwszComponentCode /*[40]*/,
                                                              DWORD *poffArguments);
            PFNMSIDECOMPOSEDESCRIPTORW pfnMsiDecomposeDescriptorW;
            pfnMsiDecomposeDescriptorW = (PFNMSIDECOMPOSEDESCRIPTORW)RTLdrGetSystemSymbol("msi.dll", "MsiDecomposeDescriptorW");
            if (   pfnMsiDecomposeDescriptorW
                && (   dwType == REG_SZ
                    || dwType == REG_MULTI_SZ))
            {
                wchar_t wszProductCode[RTUUID_STR_LENGTH + 2 + 16]   = { 0 };
                wchar_t wszFeatureId[RTUUID_STR_LENGTH + 2 + 16]     = { 0 };
                wchar_t wszComponentCode[RTUUID_STR_LENGTH + 2 + 16] = { 0 };
                DWORD   offArguments = ~(DWORD)0;
                UINT uRc = pfnMsiDecomposeDescriptorW(wszBuf, wszProductCode, wszFeatureId, wszComponentCode, &offArguments);
                if (uRc == 0)
                {
                    /*
                     * Can we resolve the product code into a name?
                     */
                    typedef UINT (WINAPI *PFNMSIOPENPRODUCTW)(PCWSTR, MSIHANDLE *);
                    PFNMSIOPENPRODUCTW pfnMsiOpenProductW;
                    pfnMsiOpenProductW = (PFNMSIOPENPRODUCTW)RTLdrGetSystemSymbol("msi.dll", "MsiOpenProductW");

                    typedef UINT (WINAPI *PFNMSICLOSEHANDLE)(MSIHANDLE);
                    PFNMSICLOSEHANDLE pfnMsiCloseHandle;
                    pfnMsiCloseHandle = (PFNMSICLOSEHANDLE)RTLdrGetSystemSymbol("msi.dll", "MsiCloseHandle");

                    typedef UINT (WINAPI *PFNGETPRODUCTPROPERTYW)(MSIHANDLE, PCWSTR, PWSTR, PDWORD);
                    PFNGETPRODUCTPROPERTYW pfnMsiGetProductPropertyW;
                    pfnMsiGetProductPropertyW = (PFNGETPRODUCTPROPERTYW)RTLdrGetSystemSymbol("msi.dll", "MsiGetProductPropertyW");
                    if (   pfnMsiGetProductPropertyW
                        && pfnMsiCloseHandle
                        && pfnMsiOpenProductW)
                    {
                        MSIHANDLE hMsi = 0;
                        uRc = pfnMsiOpenProductW(wszProductCode, &hMsi);
                        if (uRc == 0)
                        {
                            static wchar_t const * const s_apwszProps[] =
                            {
                                INSTALLPROPERTY_INSTALLEDPRODUCTNAME,
                                INSTALLPROPERTY_PRODUCTNAME,
                                INSTALLPROPERTY_PACKAGENAME,
                            };

                            wchar_t  wszProductName[1024];
                            DWORD    cwcProductName;
                            unsigned i = 0;
                            do
                            {
                                cwcProductName = RT_ELEMENTS(wszProductName) - 1;
                                uRc = pfnMsiGetProductPropertyW(hMsi, s_apwszProps[i], wszProductName, &cwcProductName);
                            }
                            while (   ++i < RT_ELEMENTS(s_apwszProps)
                                   && (   uRc != 0
                                       || cwcProductName < 2
                                       || cwcProductName >= RT_ELEMENTS(wszProductName)) );
                            uRc = pfnMsiCloseHandle(hMsi);
                            if (uRc == 0 && cwcProductName >= 2)
                            {
                                wszProductName[RT_MIN(cwcProductName, RT_ELEMENTS(wszProductName) - 1)] = '\0';
                                setError(hrcCaller,
                                         tr("Failed to instantiate CLSID_VirtualPox w/ IVirtualPox, but CLSID_VirtualPox w/ IUnknown works.\n"
                                            "PSDispatch looks broken by the '%ls' (%ls) program, suspecting that it features the broken oleaut32.msm module as component %ls.\n"
                                            "\n"
                                            "We suggest you try uninstall '%ls'.\n"
                                            "\n"
                                            "See also https://support.microsoft.com/en-us/kb/316911 "),
                                         wszProductName, wszProductCode, wszComponentCode, wszProductName);
                                fSetError = true;
                            }
                        }
                    }

                    /* MSI uses COM and may mess up our stuff. So, we wait with the fallback till afterwards in this case. */
                    if (!fSetError)
                    {
                        setError(hrcCaller,
                                 tr("Failed to instantiate CLSID_VirtualPox w/ IVirtualPox, CLSID_VirtualPox w/ IUnknown works.\n"
                                    "PSDispatch looks broken by installer %ls featuring the broken oleaut32.msm module as component %ls.\n"
                                    "\n"
                                    "See also https://support.microsoft.com/en-us/kb/316911 "),
                                 wszProductCode, wszComponentCode);
                        fSetError = true;
                    }
                }
            }
            if (!fSetError)
                setError(hrcCaller, tr("Failed to instantiate CLSID_VirtualPox w/ IVirtualPox, CLSID_VirtualPox w/ IUnknown works.\n"
                                       "PSDispatch looks broken by some installer featuring the broken oleaut32.msm module as a component.\n"
                                       "\n"
                                       "See also https://support.microsoft.com/en-us/kb/316911 "));
        }
        else if (lrc == ERROR_FILE_NOT_FOUND)
            setError(hrcCaller, tr("Failed to instantiate CLSID_VirtualPox w/ IVirtualPox, but CLSID_VirtualPox w/ IUnknown works.\n"
                                   "PSDispatch looks fine. Weird"));
        else
            setError(hrcCaller, tr("Failed to instantiate CLSID_VirtualPox w/ IVirtualPox, but CLSID_VirtualPox w/ IUnknown works.\n"
                                   "Checking out PSDispatch registration ended with error: %u (%#x)"), lrc, lrc);
        RegCloseKey(hKey);
    }

    pUnknown->Release();
    return hrcCaller;
}

# ifdef VPOX_WITH_SDS
/**
 * Gets the service account name and start type for the given service.
 *
 * @returns IPRT status code (for some reason).
 * @param   pwszServiceName The name of the service.
 * @param   pwszAccountName Where to return the account name.
 * @param   cwcAccountName  The length of the account name buffer (in WCHARs).
 * @param   puStartType     Where to return the start type.
 */
int VirtualPoxClient::i_getServiceAccountAndStartType(const wchar_t *pwszServiceName,
                                                      wchar_t *pwszAccountName, size_t cwcAccountName, uint32_t *puStartType)
{
    AssertPtr(pwszServiceName);
    AssertPtr(pwszAccountName);
    Assert(cwcAccountName);
    *pwszAccountName = '\0';
    *puStartType     = SERVICE_DEMAND_START;

    int vrc;

    // Get a handle to the SCM database.
    SC_HANDLE hSCManager = OpenSCManagerW(NULL /*pwszMachineName*/, NULL /*pwszDatabaseName*/, SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
    {
        SC_HANDLE hService = OpenServiceW(hSCManager, pwszServiceName, SERVICE_QUERY_CONFIG);
        if (hService != NULL)
        {
            DWORD cbNeeded = sizeof(QUERY_SERVICE_CONFIGW) + _1K;
            if (!QueryServiceConfigW(hService, NULL, 0, &cbNeeded))
            {
                Assert(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
                LPQUERY_SERVICE_CONFIGW pSc = (LPQUERY_SERVICE_CONFIGW)RTMemTmpAllocZ(cbNeeded + _1K);
                if (pSc)
                {
                    DWORD cbNeeded2 = 0;
                    if (QueryServiceConfigW(hService, pSc, cbNeeded + _1K, &cbNeeded2))
                    {
                        *puStartType = pSc->dwStartType;
                        vrc = RTUtf16Copy(pwszAccountName, cwcAccountName, pSc->lpServiceStartName);
                        if (RT_FAILURE(vrc))
                            LogRel(("Error: SDS service name is too long (%Rrc): %ls\n", vrc, pSc->lpServiceStartName));
                    }
                    else
                    {
                        int dwError = GetLastError();
                        vrc = RTErrConvertFromWin32(dwError);
                        LogRel(("Error: Failed querying '%ls' service config: %Rwc (%u) -> %Rrc; cbNeeded=%d cbNeeded2=%d\n",
                                pwszServiceName, dwError, dwError, vrc, cbNeeded, cbNeeded2));
                    }
                    RTMemTmpFree(pSc);
                }
                else
                {
                    LogRel(("Error: Failed allocating %#x bytes of memory for service config!\n", cbNeeded + _1K));
                    vrc = VERR_NO_TMP_MEMORY;
                }
            }
            else
            {
                AssertLogRelMsgFailed(("Error: QueryServiceConfigW returns success with zero buffer!\n"));
                vrc = VERR_IPE_UNEXPECTED_STATUS;
            }
            CloseServiceHandle(hService);
        }
        else
        {
            int dwError = GetLastError();
            vrc = RTErrConvertFromWin32(dwError);
            LogRel(("Error: Could not open service '%ls': %Rwc (%u) -> %Rrc\n", pwszServiceName, dwError, dwError, vrc));
        }
        CloseServiceHandle(hSCManager);
    }
    else
    {
        int dwError = GetLastError();
        vrc = RTErrConvertFromWin32(dwError);
        LogRel(("Error: Could not open SCM: %Rwc (%u) -> %Rrc\n", dwError, dwError, vrc));
    }
    return vrc;
}
# endif /* VPOX_WITH_SDS */

#endif /* RT_OS_WINDOWS */

/**
 *  Uninitializes the instance and sets the ready flag to FALSE.
 *  Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void VirtualPoxClient::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    if (mData.m_ThreadWatcher != NIL_RTTHREAD)
    {
        /* Signal the event semaphore and wait for the thread to terminate.
         * if it hangs for some reason exit anyway, this can cause a crash
         * though as the object will no longer be available. */
        RTSemEventSignal(mData.m_SemEvWatcher);
        RTThreadWait(mData.m_ThreadWatcher, 30000, NULL);
        mData.m_ThreadWatcher = NIL_RTTHREAD;
        RTSemEventDestroy(mData.m_SemEvWatcher);
        mData.m_SemEvWatcher = NIL_RTSEMEVENT;
    }

    mData.m_pToken.setNull();
    mData.m_pVirtualPox.setNull();

    ASMAtomicDecU32(&g_cInstances);
}

// IVirtualPoxClient properties
/////////////////////////////////////////////////////////////////////////////

/**
 * Returns a reference to the VirtualPox object.
 *
 * @returns COM status code
 * @param   aVirtualPox Address of result variable.
 */
HRESULT VirtualPoxClient::getVirtualPox(ComPtr<IVirtualPox> &aVirtualPox)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aVirtualPox = mData.m_pVirtualPox;
    return S_OK;
}

/**
 * Create a new Session object and return a reference to it.
 *
 * @returns COM status code
 * @param   aSession    Address of result variable.
 */
HRESULT VirtualPoxClient::getSession(ComPtr<ISession> &aSession)
{
    /* this is not stored in this object, no need to lock */
    ComPtr<ISession> pSession;
    HRESULT rc = pSession.createInprocObject(CLSID_Session);
    if (SUCCEEDED(rc))
        aSession = pSession;
    return rc;
}

/**
 * Return reference to the EventSource associated with this object.
 *
 * @returns COM status code
 * @param   aEventSource    Address of result variable.
 */
HRESULT VirtualPoxClient::getEventSource(ComPtr<IEventSource> &aEventSource)
{
    /* this is const, no need to lock */
    aEventSource = mData.m_pEventSource;
    return aEventSource.isNull() ? E_FAIL : S_OK;
}

// IVirtualPoxClient methods
/////////////////////////////////////////////////////////////////////////////

/**
 * Checks a Machine object for any pending errors.
 *
 * @returns COM status code
 * @param   aMachine    Machine object to check.
 */
HRESULT VirtualPoxClient::checkMachineError(const ComPtr<IMachine> &aMachine)
{
    BOOL fAccessible = FALSE;
    HRESULT rc = aMachine->COMGETTER(Accessible)(&fAccessible);
    if (FAILED(rc))
        return setError(rc, tr("Could not check the accessibility status of the VM"));
    else if (!fAccessible)
    {
        ComPtr<IVirtualPoxErrorInfo> pAccessError;
        rc = aMachine->COMGETTER(AccessError)(pAccessError.asOutParam());
        if (FAILED(rc))
            return setError(rc, tr("Could not get the access error message of the VM"));
        else
        {
            ErrorInfo info(pAccessError);
            ErrorInfoKeeper eik(info);
            return info.getResultCode();
        }
    }
    return S_OK;
}

// private methods
/////////////////////////////////////////////////////////////////////////////


/// @todo AM Add pinging of VPoxSDS
/*static*/
DECLCALLBACK(int) VirtualPoxClient::SVCWatcherThread(RTTHREAD ThreadSelf,
                                                     void *pvUser)
{
    NOREF(ThreadSelf);
    Assert(pvUser);
    VirtualPoxClient *pThis = (VirtualPoxClient *)pvUser;
    RTSEMEVENT sem = pThis->mData.m_SemEvWatcher;
    RTMSINTERVAL cMillies = VPOXCLIENT_DEFAULT_INTERVAL;
    int vrc;

    /* The likelihood of early crashes are high, so start with a short wait. */
    vrc = RTSemEventWait(sem, cMillies / 2);

    /* As long as the waiting times out keep retrying the wait. */
    while (RT_FAILURE(vrc))
    {
        {
            HRESULT rc = S_OK;
            ComPtr<IVirtualPox> pV;
            {
                AutoReadLock alock(pThis COMMA_LOCKVAL_SRC_POS);
                pV = pThis->mData.m_pVirtualPox;
            }
            if (!pV.isNull())
            {
                ULONG rev;
                rc = pV->COMGETTER(Revision)(&rev);
                if (FAILED_DEAD_INTERFACE(rc))
                {
                    LogRel(("VirtualPoxClient: detected unresponsive VPoxSVC (rc=%Rhrc)\n", rc));
                    {
                        AutoWriteLock alock(pThis COMMA_LOCKVAL_SRC_POS);
                        /* Throw away the VirtualPox reference, it's no longer
                         * usable as VPoxSVC terminated in the mean time. */
                        pThis->mData.m_pVirtualPox.setNull();
                    }
                    fireVPoxSVCAvailabilityChangedEvent(pThis->mData.m_pEventSource, FALSE);
                }
            }
            else
            {
                /* Try to get a new VirtualPox reference straight away, and if
                 * this fails use an increased waiting time as very frequent
                 * restart attempts in some wedged config can cause high CPU
                 * and disk load. */
                ComPtr<IVirtualPox> pVirtualPox;
                ComPtr<IToken> pToken;
                rc = pVirtualPox.createLocalObject(CLSID_VirtualPox);
                if (FAILED(rc))
                    cMillies = 3 * VPOXCLIENT_DEFAULT_INTERVAL;
                else
                {
                    LogRel(("VirtualPoxClient: detected working VPoxSVC (rc=%Rhrc)\n", rc));
                    {
                        AutoWriteLock alock(pThis COMMA_LOCKVAL_SRC_POS);
                        /* Update the VirtualPox reference, there's a working
                         * VPoxSVC again from now on. */
                        pThis->mData.m_pVirtualPox = pVirtualPox;
                        pThis->mData.m_pToken = pToken;
                    }
                    fireVPoxSVCAvailabilityChangedEvent(pThis->mData.m_pEventSource, TRUE);
                    cMillies = VPOXCLIENT_DEFAULT_INTERVAL;
                }
            }
        }
        vrc = RTSemEventWait(sem, cMillies);
    }
    return 0;
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
