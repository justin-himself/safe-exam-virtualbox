/* $Id: VPoxWddmUmHlp.h $ */
/** @file
 * VPox WDDM User Mode Driver Helpers
 */

/*
 * Copyright (C) 2018-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef GA_INCLUDED_SRC_3D_win_VPoxWddmUmHlp_VPoxWddmUmHlp_h
#define GA_INCLUDED_SRC_3D_win_VPoxWddmUmHlp_VPoxWddmUmHlp_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/win/d3d9.h>
#include <d3dumddi.h>
#include <d3dkmthk.h>

#include <iprt/asm.h>
#include <iprt/cdefs.h>

/* Do not require IPRT library. */
#if defined(Assert)
#undef Assert
#endif
#ifdef RT_STRICT
#define Assert(_e) (void)( (!!(_e)) || (ASMBreakpoint(), 0) )
#else
#define Assert(_e) (void)( 0 )
#endif

/* Do not require ntstatus.h.
 * D3DKMT functions return NTSTATUS, but the driver code uses it only as a success/failure indicator.
 * Therefore define the success and a failure status here.
 */
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif
#ifndef STATUS_NOT_SUPPORTED
#define STATUS_NOT_SUPPORTED ((NTSTATUS)0xC00000BBL)
#endif

RT_C_DECLS_BEGIN

typedef struct VPOXWDDMDLLPROC
{
    char const *pszName;
    FARPROC *ppfn;
} VPOXWDDMDLLPROC;

typedef struct D3DKMTFUNCTIONS
{
    PFND3DKMT_OPENADAPTERFROMHDC         pfnD3DKMTOpenAdapterFromHdc;
    PFND3DKMT_OPENADAPTERFROMDEVICENAME  pfnD3DKMTOpenAdapterFromDeviceName;
    PFND3DKMT_CLOSEADAPTER               pfnD3DKMTCloseAdapter;
    PFND3DKMT_QUERYADAPTERINFO           pfnD3DKMTQueryAdapterInfo;
    PFND3DKMT_ESCAPE                     pfnD3DKMTEscape;
    PFND3DKMT_CREATEDEVICE               pfnD3DKMTCreateDevice;
    PFND3DKMT_DESTROYDEVICE              pfnD3DKMTDestroyDevice;
    PFND3DKMT_CREATECONTEXT              pfnD3DKMTCreateContext;
    PFND3DKMT_DESTROYCONTEXT             pfnD3DKMTDestroyContext;
    PFND3DKMT_CREATEALLOCATION           pfnD3DKMTCreateAllocation;
    PFND3DKMT_DESTROYALLOCATION          pfnD3DKMTDestroyAllocation;
    PFND3DKMT_RENDER                     pfnD3DKMTRender;
    PFND3DKMT_PRESENT                    pfnD3DKMTPresent;
    PFND3DKMT_GETSHAREDPRIMARYHANDLE     pfnD3DKMTGetSharedPrimaryHandle;
    PFND3DKMT_QUERYRESOURCEINFO          pfnD3DKMTQueryResourceInfo;
    PFND3DKMT_OPENRESOURCE               pfnD3DKMTOpenResource;

    /* Win 8+ */
    PFND3DKMT_ENUMADAPTERS               pfnD3DKMTEnumAdapters;
    PFND3DKMT_OPENADAPTERFROMLUID        pfnD3DKMTOpenAdapterFromLuid;
} D3DKMTFUNCTIONS;

DECLCALLBACK(HMODULE) VPoxWddmLoadSystemDll(const char *pszName);
DECLCALLBACK(void) VPoxWddmLoadAdresses(HMODULE hmod, VPOXWDDMDLLPROC *paProcs);

DECLCALLBACK(int) D3DKMTLoad(void);
DECLCALLBACK(D3DKMTFUNCTIONS const *) D3DKMTFunctions(void);

DECLCALLBACK(void) VPoxDispMpLoggerLogF(const char *pszString, ...);
DECLCALLBACK(void) VPoxWddmUmLog(const char *pszString);

/** @todo Rename to VPoxWddm* */
NTSTATUS vpoxDispKmtOpenAdapter2(D3DKMT_HANDLE *phAdapter, LUID *pLuid);
NTSTATUS vpoxDispKmtOpenAdapter(D3DKMT_HANDLE *phAdapter);
NTSTATUS vpoxDispKmtCloseAdapter(D3DKMT_HANDLE hAdapter);

RT_C_DECLS_END

#endif /* !GA_INCLUDED_SRC_3D_win_VPoxWddmUmHlp_VPoxWddmUmHlp_h */
