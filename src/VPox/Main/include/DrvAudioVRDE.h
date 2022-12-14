/* $Id: DrvAudioVRDE.h $ */
/** @file
 * VirtualPox driver interface to VRDE backend.
 */

/*
 * Copyright (C) 2014-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_DrvAudioVRDE_h
#define MAIN_INCLUDED_DrvAudioVRDE_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VPox/com/ptr.h>
#include <VPox/com/string.h>

#include <VPox/RemoteDesktop/VRDE.h>

#include <VPox/vmm/pdmdrv.h>
#include <VPox/vmm/pdmifs.h>

#include "AudioDriver.h"

using namespace com;

class Console;

class AudioVRDE : public AudioDriver
{

public:

    AudioVRDE(Console *pConsole);
    virtual ~AudioVRDE(void);

public:

    static const PDMDRVREG DrvReg;

public:

    void onVRDEClientConnect(uint32_t uClientID);
    void onVRDEClientDisconnect(uint32_t uClientID);
    int onVRDEControl(bool fEnable, uint32_t uFlags);
    int onVRDEInputBegin(void *pvContext, PVRDEAUDIOINBEGIN pVRDEAudioBegin);
    int onVRDEInputData(void *pvContext, const void *pvData, uint32_t cbData);
    int onVRDEInputEnd(void *pvContext);
    int onVRDEInputIntercept(bool fIntercept);

public:

    static DECLCALLBACK(int) drvConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags);
    static DECLCALLBACK(void) drvDestruct(PPDMDRVINS pDrvIns);
    static DECLCALLBACK(void) drvPowerOff(PPDMDRVINS pDrvIns);

private:

    int configureDriver(PCFGMNODE pLunCfg);

    /** Pointer to the associated VRDE audio driver. */
    struct DRVAUDIOVRDE *mpDrv;
    /** Protects accesses to mpDrv from racing driver destruction. */
    RTCRITSECT mCritSect;
};

#endif /* !MAIN_INCLUDED_DrvAudioVRDE_h */

