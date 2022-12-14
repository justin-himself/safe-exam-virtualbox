/* $Id: netif.h $ */
/** @file
 * Main - Network Interfaces.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_netif_h
#define MAIN_INCLUDED_netif_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/net.h>
/** @todo r=bird: The inlined code below that drags in asm.h here. I doubt
 *        speed is very important here, so move it into a .cpp file, please. */
#include <iprt/asm.h>

#ifndef RT_OS_WINDOWS
# include <arpa/inet.h>
# include <stdio.h>
#endif /* !RT_OS_WINDOWS */

#define VPOXNET_IPV4ADDR_DEFAULT      0x0138A8C0  /* 192.168.56.1 */
#define VPOXNET_IPV4MASK_DEFAULT      "255.255.255.0"

#define VPOXNET_MAX_SHORT_NAME 50

#if 1
/**
 * Encapsulation type.
 * @note Must match HostNetworkInterfaceMediumType_T exactly.
 * @todo r=bird: Why are we duplicating HostNetworkInterfaceMediumType_T here?!?
 */
typedef enum NETIFTYPE
{
    NETIF_T_UNKNOWN,
    NETIF_T_ETHERNET,
    NETIF_T_PPP,
    NETIF_T_SLIP
} NETIFTYPE;

/**
 * Current state of the interface.
 * @note Must match HostNetworkInterfaceStatus_T exactly.
 * @todo r=bird: Why are we duplicating HostNetworkInterfaceStatus_T here?!?
 */
typedef enum NETIFSTATUS
{
    NETIF_S_UNKNOWN,
    NETIF_S_UP,
    NETIF_S_DOWN
} NETIFSTATUS;

/**
 * Host Network Interface Information.
 */
typedef struct NETIFINFO
{
    NETIFINFO     *pNext;
    RTNETADDRIPV4  IPAddress;
    RTNETADDRIPV4  IPNetMask;
    RTNETADDRIPV6  IPv6Address;
    RTNETADDRIPV6  IPv6NetMask;
    BOOL           fDhcpEnabled;
    BOOL           fIsDefault;
    BOOL           fWireless;
    RTMAC          MACAddress;
    NETIFTYPE      enmMediumType;
    NETIFSTATUS    enmStatus;
    uint32_t       uSpeedMbits;
    RTUUID         Uuid;
    char           szShortName[VPOXNET_MAX_SHORT_NAME];
    char           szName[1];
} NETIFINFO;

/** Pointer to a network interface info. */
typedef NETIFINFO *PNETIFINFO;
/** Pointer to a const network interface info. */
typedef NETIFINFO const *PCNETIFINFO;
#endif

int NetIfList(std::list <ComObjPtr<HostNetworkInterface> > &list);
int NetIfEnableStaticIpConfig(VirtualPox *pVPox, HostNetworkInterface * pIf, ULONG aOldIp, ULONG aNewIp, ULONG aMask);
int NetIfEnableStaticIpConfigV6(VirtualPox *pVPox, HostNetworkInterface *pIf, const Utf8Str &aOldIPV6Address, const Utf8Str &aIPV6Address, ULONG aIPV6MaskPrefixLength);
int NetIfEnableDynamicIpConfig(VirtualPox *pVPox, HostNetworkInterface * pIf);
#ifdef RT_OS_WINDOWS
int NetIfCreateHostOnlyNetworkInterface(VirtualPox *pVPox, IHostNetworkInterface **aHostNetworkInterface, IProgress **aProgress,
                                        IN_BSTR bstrName = NULL);
#else
int NetIfCreateHostOnlyNetworkInterface(VirtualPox *pVPox, IHostNetworkInterface **aHostNetworkInterface, IProgress **aProgress,
                                        const char *pszName = NULL);
#endif
int NetIfRemoveHostOnlyNetworkInterface(VirtualPox *pVPox, const Guid &aId, IProgress **aProgress);
int NetIfGetConfig(HostNetworkInterface * pIf, NETIFINFO *);
int NetIfGetConfigByName(PNETIFINFO pInfo);
int NetIfGetState(const char *pcszIfName, NETIFSTATUS *penmState);
int NetIfGetLinkSpeed(const char *pcszIfName, uint32_t *puMbits);
int NetIfDhcpRediscover(VirtualPox *pVPox, HostNetworkInterface * pIf);
int NetIfAdpCtlOut(const char *pszName, const char *pszCmd, char *pszBuffer, size_t cBufSize);

DECLINLINE(Bstr) getDefaultIPv4Address(Bstr bstrIfName)
{
    /* Get the index from the name */
    Utf8Str strTmp = bstrIfName;
    const char *pcszIfName = strTmp.c_str();
    int iInstance = 0;
    size_t iPos = strcspn(pcszIfName, "0123456789");
    if (pcszIfName[iPos])
        iInstance = RTStrToUInt32(pcszIfName + iPos);

    in_addr tmp;
#if defined(RT_OS_WINDOWS)
    tmp.S_un.S_addr = VPOXNET_IPV4ADDR_DEFAULT + (iInstance << 16);
#else
    tmp.s_addr = VPOXNET_IPV4ADDR_DEFAULT + (iInstance << 16);
#endif
    char *addr = inet_ntoa(tmp);
    return Bstr(addr);
}

#endif /* !MAIN_INCLUDED_netif_h */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
