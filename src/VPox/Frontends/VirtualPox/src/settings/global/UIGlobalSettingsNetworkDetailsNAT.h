/* $Id: UIGlobalSettingsNetworkDetailsNAT.h $ */
/** @file
 * VPox Qt GUI - UIGlobalSettingsNetworkDetailsNAT class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsNetworkDetailsNAT_h
#define FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsNetworkDetailsNAT_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIDialog.h"
#include "QIWithRetranslateUI.h"
#include "UIGlobalSettingsNetworkDetailsNAT.gen.h"
#include "UIPortForwardingTable.h"


/** Global settings: Network page: NAT network data structure. */
struct UIDataSettingsGlobalNetworkNAT
{
    /** Constructs data. */
    UIDataSettingsGlobalNetworkNAT()
        : m_fEnabled(false)
        , m_strName(QString())
        , m_strNewName(QString())
        , m_strCIDR(QString())
        , m_fSupportsDHCP(false)
        , m_fSupportsIPv6(false)
        , m_fAdvertiseDefaultIPv6Route(false)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsGlobalNetworkNAT &other) const
    {
        return true
               && (m_fEnabled == other.m_fEnabled)
               && (m_strName == other.m_strName)
               && (m_strNewName == other.m_strNewName)
               && (m_strCIDR == other.m_strCIDR)
               && (m_fSupportsDHCP == other.m_fSupportsDHCP)
               && (m_fSupportsIPv6 == other.m_fSupportsIPv6)
               && (m_fAdvertiseDefaultIPv6Route == other.m_fAdvertiseDefaultIPv6Route)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsGlobalNetworkNAT &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsGlobalNetworkNAT &other) const { return !equal(other); }

    /** Holds whether this network enabled. */
    bool m_fEnabled;
    /** Holds network name. */
    QString m_strName;
    /** Holds new network name. */
    QString m_strNewName;
    /** Holds network CIDR. */
    QString m_strCIDR;
    /** Holds whether this network supports DHCP. */
    bool m_fSupportsDHCP;
    /** Holds whether this network supports IPv6. */
    bool m_fSupportsIPv6;
    /** Holds whether this network advertised as default IPv6 route. */
    bool m_fAdvertiseDefaultIPv6Route;
};


/* Global settings / Network page / Details sub-dialog: */
class SHARED_LIBRARY_STUFF UIGlobalSettingsNetworkDetailsNAT : public QIWithRetranslateUI2<QIDialog>, public Ui::UIGlobalSettingsNetworkDetailsNAT
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIGlobalSettingsNetworkDetailsNAT(QWidget *pParent, UIDataSettingsGlobalNetworkNAT &data, UIPortForwardingDataList &ipv4rules, UIPortForwardingDataList &ipv6rules);

protected:

    /* Handler: Translation stuff: */
    void retranslateUi();

    /* Handler: Polish event: */
    void polishEvent(QShowEvent *pEvent);

private slots:

    /* Handler: Port-forwarding stuff: */
    void sltEditPortForwarding();

    /* Handler: Dialog stuff: */
    void accept();

private:

    /* Helpers: Load/Save stuff: */
    void load();
    void save();

    /* Variable: External data reference: */
    UIDataSettingsGlobalNetworkNAT &m_data;

    /** Holds IPv4 port forwarding rules. */
    UIPortForwardingDataList &m_ipv4rules;
    /** Holds IPv6 port forwarding rules. */
    UIPortForwardingDataList &m_ipv6rules;
};


#endif /* !FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsNetworkDetailsNAT_h */
