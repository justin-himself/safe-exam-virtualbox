/** @file
 *
 * VPox frontends: Framebuffer (FB, DirectFB):
 * Main header file
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

#ifndef VPOX_INCLUDED_SRC_VPoxFB_VPoxFB_h
#define VPOX_INCLUDED_SRC_VPoxFB_VPoxFB_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// XPCOM headers
#include <nsIServiceManager.h>
#include <nsIComponentRegistrar.h>
#include <nsXPCOMGlue.h>
#include <nsMemory.h>
#include <nsIProgrammingLanguage.h>
#include <nsIFile.h>
#include <nsILocalFile.h>
#include <nsString.h>
#include <nsReadableUtils.h>
#include <VirtualPox_XPCOM.h>
#include <ipcIService.h>
#include <nsEventQueueUtils.h>
#include <ipcCID.h>
#include <ipcIDConnectService.h>
#define IPC_DCONNECTSERVICE_CONTRACTID \
    "@mozilla.org/ipc/dconnect-service;1"

#include <VPox/types.h>
#include <VPox/err.h>
#include <VPox/log.h>
#include <iprt/assert.h>
#include <iprt/uuid.h>

// DirectFB header
#include <directfb/directfb.h>

/**
 * Executes the passed in expression and verifies the return code.
 *
 * On failure a debug message is printed to stderr and the application will
 * abort with an fatal error.
 */
#define DFBCHECK(x...) \
    do { \
        DFBResult err = x; \
        if (err != DFB_OK) \
        { \
            fprintf(stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
            DirectFBErrorFatal(#x, err); \
        } \
    } while (0)

#include "Helper.h"

/**
 * Globals
 */
extern uint32_t useFixedVideoMode;
extern videoMode fixedVideoMode;
extern int scaleGuest;

#endif /* !VPOX_INCLUDED_SRC_VPoxFB_VPoxFB_h */
