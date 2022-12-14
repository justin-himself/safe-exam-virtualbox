/* $Id: Helper.h $ */
/** @file
 *
 * VPox frontends: VPoxSDL (simple frontend based on SDL):
 * Miscellaneous helpers header
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

#ifndef VPOX_INCLUDED_SRC_VPoxSDL_Helper_h
#define VPOX_INCLUDED_SRC_VPoxSDL_Helper_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#if defined(VPOX_WITH_XPCOM) && !defined(RT_OS_DARWIN) && !defined(RT_OS_OS2)

/** Indicates that the XPCOM queue thread is needed for this platform. */
# define USE_XPCOM_QUEUE_THREAD 1

/**
 * Creates the XPCOM event thread
 *
 * @returns VPOX status code
 * @param   eqFD XPCOM event queue file descriptor
 */
int startXPCOMEventQueueThread(int eqFD);

/*
 * Notify the XPCOM thread that we consumed an XPCOM event
 */
void consumedXPCOMUserEvent(void);

/**
 * Signal to the XPCOM even queue thread that it should select for more events.
 */
void signalXPCOMEventQueueThread(void);

/**
 * Indicates to the XPCOM thread that it should terminate now.
 */
void terminateXPCOMQueueThread(void);

#endif /* defined(VPOX_WITH_XPCOM) && !defined(RT_OS_DARWIN) && !defined(RT_OS_OS2) */

#endif /* !VPOX_INCLUDED_SRC_VPoxSDL_Helper_h */

