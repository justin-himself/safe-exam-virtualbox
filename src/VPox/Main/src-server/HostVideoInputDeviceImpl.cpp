/* $Id: HostVideoInputDeviceImpl.cpp $ */
/** @file
 * Host video capture device implementation.
 */

/*
 * Copyright (C) 2013-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_MAIN_HOSTVIDEOINPUTDEVICE
#include "HostVideoInputDeviceImpl.h"
#include "LoggingNew.h"
#include "VirtualPoxImpl.h"
#ifdef VPOX_WITH_EXTPACK
# include "ExtPackManagerImpl.h"
#endif

#include <iprt/err.h>
#include <iprt/ldr.h>
#include <iprt/path.h>

#include <VPox/sup.h>

/*
 * HostVideoInputDevice implementation.
 */
DEFINE_EMPTY_CTOR_DTOR(HostVideoInputDevice)

HRESULT HostVideoInputDevice::FinalConstruct()
{
    return BaseFinalConstruct();
}

void HostVideoInputDevice::FinalRelease()
{
    uninit();

    BaseFinalRelease();
}

/*
 * Initializes the instance.
 */
HRESULT HostVideoInputDevice::init(const com::Utf8Str &name, const com::Utf8Str &path, const com::Utf8Str &alias)
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m.name = name;
    m.path = path;
    m.alias = alias;

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/*
 * Uninitializes the instance.
 * Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void HostVideoInputDevice::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    m.name.setNull();
    m.path.setNull();
    m.alias.setNull();
}

static HRESULT hostVideoInputDeviceAdd(HostVideoInputDeviceList *pList,
                                       const com::Utf8Str &name,
                                       const com::Utf8Str &path,
                                       const com::Utf8Str &alias)
{
    ComObjPtr<HostVideoInputDevice> obj;
    HRESULT hr = obj.createObject();
    if (SUCCEEDED(hr))
    {
        hr = obj->init(name, path, alias);
        if (SUCCEEDED(hr))
            pList->push_back(obj);
    }
    return hr;
}

static DECLCALLBACK(int) hostWebcamAdd(void *pvUser,
                                       const char *pszName,
                                       const char *pszPath,
                                       const char *pszAlias,
                                       uint64_t *pu64Result)
{
    HostVideoInputDeviceList *pList = (HostVideoInputDeviceList *)pvUser;
    HRESULT hr = hostVideoInputDeviceAdd(pList, pszName, pszPath, pszAlias);
    if (FAILED(hr))
    {
        *pu64Result = (uint64_t)hr;
        return VERR_NOT_SUPPORTED;
    }
    return VINF_SUCCESS;
}

/** @todo These typedefs must be in a header. */
typedef DECLCALLBACK(int) FNVPOXHOSTWEBCAMADD(void *pvUser,
                                              const char *pszName,
                                              const char *pszPath,
                                              const char *pszAlias,
                                              uint64_t *pu64Result);
typedef FNVPOXHOSTWEBCAMADD *PFNVPOXHOSTWEBCAMADD;

typedef DECLCALLBACK(int) FNVPOXHOSTWEBCAMLIST(PFNVPOXHOSTWEBCAMADD pfnWebcamAdd,
                                               void *pvUser,
                                               uint64_t *pu64WebcamAddResult);
typedef FNVPOXHOSTWEBCAMLIST *PFNVPOXHOSTWEBCAMLIST;

static int loadHostWebcamLibrary(const char *pszPath, RTLDRMOD *phmod, PFNVPOXHOSTWEBCAMLIST *ppfn)
{
    int rc;
    if (RTPathHavePath(pszPath))
    {
        RTLDRMOD hmod = NIL_RTLDRMOD;
        RTERRINFOSTATIC ErrInfo;
        rc = SUPR3HardenedLdrLoadPlugIn(pszPath, &hmod, RTErrInfoInitStatic(&ErrInfo));
        if (RT_SUCCESS(rc))
        {
            static const char s_szSymbol[] = "VPoxHostWebcamList";
            rc = RTLdrGetSymbol(hmod, s_szSymbol, (void **)ppfn);
            if (RT_SUCCESS(rc))
                *phmod = hmod;
            else
            {
                if (rc != VERR_SYMBOL_NOT_FOUND)
                    LogRel(("Resolving symbol '%s': %Rrc\n", s_szSymbol, rc));
                RTLdrClose(hmod);
                hmod = NIL_RTLDRMOD;
            }
        }
        else
        {
            LogRel(("Loading the library '%s': %Rrc\n", pszPath, rc));
            if (RTErrInfoIsSet(&ErrInfo.Core))
                LogRel(("  %s\n", ErrInfo.Core.pszMsg));
        }
    }
    else
    {
        LogRel(("Loading the library '%s': No path! Refusing to try loading it!\n", pszPath));
        rc = VERR_INVALID_PARAMETER;
    }
    return rc;
}


static HRESULT fillDeviceList(VirtualPox *pVirtualPox, HostVideoInputDeviceList *pList)
{
    HRESULT hr;
    Utf8Str strLibrary;

#ifdef VPOX_WITH_EXTPACK
    ExtPackManager *pExtPackMgr = pVirtualPox->i_getExtPackManager();
    hr = pExtPackMgr->i_getLibraryPathForExtPack("VPoxHostWebcam", ORACLE_PUEL_EXTPACK_NAME, &strLibrary);
#else
    hr = E_NOTIMPL;
#endif

    if (SUCCEEDED(hr))
    {
        PFNVPOXHOSTWEBCAMLIST pfn = NULL;
        RTLDRMOD hmod = NIL_RTLDRMOD;
        int vrc = loadHostWebcamLibrary(strLibrary.c_str(), &hmod, &pfn);

        LogRel(("Load [%s] vrc=%Rrc\n", strLibrary.c_str(), vrc));

        if (RT_SUCCESS(vrc))
        {
            uint64_t u64Result = S_OK;
            vrc = pfn(hostWebcamAdd, pList, &u64Result);
            Log(("VPoxHostWebcamList vrc %Rrc, result 0x%08X\n", vrc, u64Result));
            if (RT_FAILURE(vrc))
            {
                hr = (HRESULT)u64Result;
            }

            RTLdrClose(hmod);
            hmod = NIL_RTLDRMOD;
        }

        if (SUCCEEDED(hr))
        {
            if (RT_FAILURE(vrc))
                hr = pVirtualPox->setErrorBoth(VPOX_E_IPRT_ERROR, vrc, "Failed to get webcam list: %Rrc", vrc);
        }
    }

    return hr;
}

/* static */ HRESULT HostVideoInputDevice::queryHostDevices(VirtualPox *pVirtualPox, HostVideoInputDeviceList *pList)
{
    HRESULT hr = fillDeviceList(pVirtualPox, pList);

    if (FAILED(hr))
    {
        pList->clear();
    }

    return hr;
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
