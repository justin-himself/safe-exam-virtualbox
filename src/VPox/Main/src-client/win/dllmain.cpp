/* $Id: dllmain.cpp $ */
/** @file
 * VPoxC - COM DLL exports and DLL init/term.
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
#include "VPox/com/defs.h"

#include <SessionImpl.h>
#include <VirtualPoxClientImpl.h>

#include <iprt/initterm.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static ATL::CComModule *g_pAtlComModule;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_Session, Session)
    OBJECT_ENTRY(CLSID_VirtualPoxClient, VirtualPoxClient)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // idempotent, so doesn't harm, and needed for COM embedding scenario
        RTR3InitDll(RTR3INIT_FLAGS_UNOBTRUSIVE);

        g_pAtlComModule = new(ATL::CComModule);
        if (!g_pAtlComModule)
            return FALSE;

        g_pAtlComModule->Init(ObjectMap, hInstance, &LIBID_VirtualPox);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (g_pAtlComModule)
        {
            g_pAtlComModule->Term();
            delete g_pAtlComModule;
            g_pAtlComModule = NULL;
        }
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    AssertReturn(g_pAtlComModule, S_OK);
    LONG const cLocks = g_pAtlComModule->GetLockCount();
    Assert(cLocks >= VirtualPoxClient::s_cUnnecessaryAtlModuleLocks);
    return cLocks <= VirtualPoxClient::s_cUnnecessaryAtlModuleLocks ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    AssertReturn(g_pAtlComModule, E_UNEXPECTED);
    return g_pAtlComModule->GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifndef VPOX_WITH_MIDL_PROXY_STUB
    // registers object, typelib and all interfaces in typelib
    AssertReturn(g_pAtlComModule, E_UNEXPECTED);
    return g_pAtlComModule->RegisterServer(TRUE);
#else
    return S_OK; /* VPoxProxyStub does all the work, no need to duplicate it here. */
#endif
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifndef VPOX_WITH_MIDL_PROXY_STUB
    AssertReturn(g_pAtlComModule, E_UNEXPECTED);
    HRESULT hrc = g_pAtlComModule->UnregisterServer(TRUE);
    return hrc;
#else
    return S_OK; /* VPoxProxyStub does all the work, no need to duplicate it here. */
#endif
}


#ifdef RT_OS_WINDOWS
/*
 * HACK ALERT! Really ugly trick to make the VirtualPoxClient object go away
 *             when nobody uses it anymore.  This is to prevent its uninit()
 *             method from accessing IVirtualPox and similar proxy stubs after
 *             COM has been officially shut down.
 *
 *             It is simply TOO LATE to destroy the client object from DllMain/detach!
 *
 *             This hack ASSUMES ObjectMap order.
 *             This hack is subject to a re-instantiation race.
 */
ULONG VirtualPoxClient::InternalRelease()
{
    ULONG cRefs = VirtualPoxClientWrap::InternalRelease();
# ifdef DEBUG_bird
    char szMsg[64];
    RTStrPrintf(szMsg, sizeof(szMsg), "VirtualPoxClient: cRefs=%d\n", cRefs);
    OutputDebugStringA(szMsg);
# endif
# if 1 /* enable ugly hack */
    if (cRefs == 1)
    {
        /* Make the factory to drop its reference. */
        if (ObjectMap[1].pCF)
        {
            InternalAddRef();

            CMyComClassFactorySingleton<VirtualPoxClient> *pFactory;
            pFactory = dynamic_cast<CMyComClassFactorySingleton<VirtualPoxClient> *>(ObjectMap[1].pCF);
            Assert(pFactory);
            if (pFactory)
            {
                IUnknown *pUnknown = pFactory->m_spObj;
                pFactory->m_spObj = NULL;
                if (pUnknown)
                    pUnknown->Release();
            }

            cRefs = VirtualPoxClientWrap::InternalRelease();
        }
    }
# endif
    return cRefs;
}
#endif

