/* $Id: VPoxHostVersion.cpp $ */
/** @file
 * VPoxHostVersion - Checks the host's VirtualPox version and notifies
 *                   the user in case of an update.
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

#include "VPoxHostVersion.h"
#include "VPoxTray.h"
#include "VPoxHelpers.h"

#include <VPox/VPoxGuestLib.h>

#ifdef DEBUG
# define LOG_ENABLED
# define LOG_GROUP LOG_GROUP_DEFAULT
#endif
#include <VPox/log.h>



/** @todo Move this part in VbglR3 and just provide a callback for the platform-specific
          notification stuff, since this is very similar to the VPoxClient code. */
int VPoxCheckHostVersion(void)
{
    int rc;
    uint32_t uGuestPropSvcClientID;

    rc = VbglR3GuestPropConnect(&uGuestPropSvcClientID);
    if (RT_SUCCESS(rc))
    {
        char *pszHostVersion;
        char *pszGuestVersion;
        bool bUpdate;
        rc = VbglR3HostVersionCheckForUpdate(uGuestPropSvcClientID, &bUpdate, &pszHostVersion, &pszGuestVersion);
        if (RT_SUCCESS(rc))
        {
            if (bUpdate)
            {
                char szMsg[256]; /* Sizes according to MSDN. */
                char szTitle[64];

                /** @todo Add some translation macros here. */
                _snprintf(szTitle, sizeof(szTitle), "VirtualPox Guest Additions update available!");
                _snprintf(szMsg, sizeof(szMsg), "Your guest is currently running the Guest Additions version %s. "
                                                "We recommend updating to the latest version (%s) by choosing the "
                                                "install option from the Devices menu.", pszGuestVersion, pszHostVersion);

                rc = hlpShowBalloonTip(g_hInstance, g_hwndToolWindow, ID_TRAYICON,
                                       szMsg, szTitle,
                                       5000 /* Time to display in msec */, NIIF_INFO);
                if (RT_FAILURE(rc))
                    LogFlowFunc(("Guest Additions update found; however: could not show version notifier balloon tooltip, rc=%Rrc\n", rc));
            }

            /* Store host version to not notify again. */
            rc = VbglR3HostVersionLastCheckedStore(uGuestPropSvcClientID, pszHostVersion);

            VbglR3GuestPropReadValueFree(pszHostVersion);
            VbglR3GuestPropReadValueFree(pszGuestVersion);
        }
        VbglR3GuestPropDisconnect(uGuestPropSvcClientID);
    }
    return rc;
}

