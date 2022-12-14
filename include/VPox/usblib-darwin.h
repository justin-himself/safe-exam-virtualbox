/** @file
 * USBLib - Library for wrapping up the VPoxUSB functionality, Darwin flavor.
 * (DEV,HDrv,Main)
 */

/*
 * Copyright (C) 2007-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualPox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef VPOX_INCLUDED_usblib_darwin_h
#define VPOX_INCLUDED_usblib_darwin_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VPox/cdefs.h>
#include <VPox/usbfilter.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_usblib_darwin     Darwin USB Specifics
 * @ingroup grp_usblib
 * @{
 */

/** @name VPoxUSB specific device properties.
 * VPoxUSB makes use of the OWNER property for communicating between the probe and
 * start stage.
 * USBProxyServiceDarwin makes use of all of them to correctly determine the state
 * of the device.
 * @{ */
/** Contains the pid of the current client. If 0, the kernel is the current client. */
#define VPOXUSB_CLIENT_KEY  "VPoxUSB-Client"
/** Contains the pid of the filter owner (i.e. the VPoxSVC pid). */
#define VPOXUSB_OWNER_KEY   "VPoxUSB-Owner"
/** Contains the ID of the matching filter. */
#define VPOXUSB_FILTER_KEY  "VPoxUSB-Filter"
/** @} */

/** @} */

RT_C_DECLS_END

#endif /* !VPOX_INCLUDED_usblib_darwin_h */

