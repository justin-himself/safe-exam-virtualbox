/* $Id: VPoxSampleDevice.cpp $ */
/** @file
 * VPox Sample Device.
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_MISC
#include <VPox/vmm/pdmdev.h>
#include <VPox/version.h>
#include <iprt/errcore.h>
#include <VPox/log.h>

#include <iprt/assert.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Device Instance Data.
 */
typedef struct VPOXSAMPLEDEVICE
{
    uint32_t    Whatever;
} VPOXSAMPLEDEVICE;
typedef VPOXSAMPLEDEVICE *PVPOXSAMPLEDEVICE;



static DECLCALLBACK(int) devSampleDestruct(PPDMDEVINS pDevIns)
{
    /*
     * Check the versions here as well since the destructor is *always* called.
     */
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

    return VINF_SUCCESS;
}


static DECLCALLBACK(int) devSampleConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    /*
     * Check that the device instance and device helper structures are compatible.
     */
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    NOREF(pCfg);

    /*
     * Initialize the instance data so that the destructor won't mess up.
     */
    PVPOXSAMPLEDEVICE pThis = PDMDEVINS_2_DATA(pDevIns, PVPOXSAMPLEDEVICE);
    pThis->Whatever = 0;

    /*
     * Validate and read the configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(pDevIns, "Whatever1|Whatever2", "");

    /*
     * Use the instance number if necessary (not for this device, which in
     * g_DeviceSample below declares a maximum instance count of 1).
     */
    NOREF(iInstance);

    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
static const PDMDEVREG g_DeviceSample =
{
    /* .u32Version = */             PDM_DEVREG_VERSION,
    /* .uReserved0 = */             0,
    /* .szName = */                 "sample",
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_NEW_STYLE,
    /* .fClass = */                 PDM_DEVREG_CLASS_MISC,
    /* .cMaxInstances = */          1,
    /* .uSharedVersion = */         42,
    /* .cbInstanceShared = */       sizeof(VPOXSAMPLEDEVICE),
    /* .cbInstanceCC = */           0,
    /* .cbInstanceRC = */           0,
    /* .cMaxPciDevices = */         0,
    /* .cMaxMsixVectors = */        0,
    /* .pszDescription = */         "VPox Sample Device.",
#if defined(IN_RING3)
    /* .pszRCMod = */               "",
    /* .pszR0Mod = */               "",
    /* .pfnConstruct = */           devSampleConstruct,
    /* .pfnDestruct = */            devSampleDestruct,
    /* .pfnRelocate = */            NULL,
    /* .pfnMemSetup = */            NULL,
    /* .pfnPowerOn = */             NULL,
    /* .pfnReset = */               NULL,
    /* .pfnSuspend = */             NULL,
    /* .pfnResume = */              NULL,
    /* .pfnAttach = */              NULL,
    /* .pfnDetach = */              NULL,
    /* .pfnQueryInterface = */      NULL,
    /* .pfnInitComplete = */        NULL,
    /* .pfnPowerOff = */            NULL,
    /* .pfnSoftReset = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RING0)
    /* .pfnEarlyConstruct = */      NULL,
    /* .pfnConstruct = */           NULL,
    /* .pfnDestruct = */            NULL,
    /* .pfnFinalDestruct = */       NULL,
    /* .pfnRequest = */             NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RC)
    /* .pfnConstruct = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#else
# error "Not in IN_RING3, IN_RING0 or IN_RC!"
#endif
    /* .u32VersionEnd = */          PDM_DEVREG_VERSION
};


/**
 * Register devices provided by the plugin.
 *
 * @returns VPox status code.
 * @param   pCallbacks      Pointer to the callback table.
 * @param   u32Version      VPox version number.
 */
extern "C" DECLEXPORT(int) VPoxDevicesRegister(PPDMDEVREGCB pCallbacks, uint32_t u32Version)
{
    LogFlow(("VPoxSampleDevice::VPoxDevicesRegister: u32Version=%#x pCallbacks->u32Version=%#x\n", u32Version, pCallbacks->u32Version));

    AssertLogRelMsgReturn(u32Version >= VPOX_VERSION,
                          ("VirtualPox version %#x, expected %#x or higher\n", u32Version, VPOX_VERSION),
                          VERR_VERSION_MISMATCH);
    AssertLogRelMsgReturn(pCallbacks->u32Version == PDM_DEVREG_CB_VERSION,
                          ("callback version %#x, expected %#x\n", pCallbacks->u32Version, PDM_DEVREG_CB_VERSION),
                          VERR_VERSION_MISMATCH);

    return pCallbacks->pfnRegister(pCallbacks, &g_DeviceSample);
}

