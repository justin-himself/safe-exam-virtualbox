/* $Id: HGCM.h $ */
/** @file
 * HGCM - Host-Guest Communication Manager.
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

#ifndef MAIN_INCLUDED_HGCM_h
#define MAIN_INCLUDED_HGCM_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VPox/vmm/pdmifs.h>

#include <VPox/hgcmsvc.h>

/* Handle of a HGCM service extension. */
struct _HGCMSVCEXTHANDLEDATA;
typedef struct _HGCMSVCEXTHANDLEDATA *HGCMSVCEXTHANDLE;

RT_C_DECLS_BEGIN
int HGCMHostInit(void);
int HGCMHostShutdown(bool fUvmIsInvalid = false);

int HGCMHostReset(bool fForShutdown);

int HGCMHostLoad(const char *pszServiceLibrary, const char *pszServiceName, PUVM pUVM, PPDMIHGCMPORT pHgcmPort);

int HGCMHostRegisterServiceExtension(HGCMSVCEXTHANDLE *pHandle, const char *pszServiceName, PFNHGCMSVCEXT pfnExtension, void *pvExtension);
void HGCMHostUnregisterServiceExtension(HGCMSVCEXTHANDLE handle);

int HGCMGuestConnect(PPDMIHGCMPORT pHGCMPort, PVPOXHGCMCMD pCmdPtr, const char *pszServiceName, uint32_t *pClientID);
int HGCMGuestDisconnect(PPDMIHGCMPORT pHGCMPort, PVPOXHGCMCMD pCmdPtr, uint32_t clientID);
int HGCMGuestCall(PPDMIHGCMPORT pHGCMPort, PVPOXHGCMCMD pCmdPtr, uint32_t clientID, uint32_t function, uint32_t cParms,
                  VPOXHGCMSVCPARM *paParms, uint64_t tsArrival);
void HGCMGuestCancelled(PPDMIHGCMPORT pHGCMPort, PVPOXHGCMCMD pCmdPtr, uint32_t idClient);

int HGCMHostCall(const char *pszServiceName, uint32_t function, uint32_t cParms, VPOXHGCMSVCPARM aParms[]);
int HGCMBroadcastEvent(HGCMNOTIFYEVENT enmEvent);

int HGCMHostSaveState(PSSMHANDLE pSSM);
int HGCMHostLoadState(PSSMHANDLE pSSM, uint32_t uVersion);

RT_C_DECLS_END

#endif /* !MAIN_INCLUDED_HGCM_h */

