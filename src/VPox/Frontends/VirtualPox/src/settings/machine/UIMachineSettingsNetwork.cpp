/* $Id: UIMachineSettingsNetwork.cpp $ */
/** @file
 * VPox Qt GUI - UIMachineSettingsNetwork class implementation.
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

/* GUI includes: */
#include "QIArrowButtonSwitch.h"
#include "QITabWidget.h"
#include "QIWidgetValidator.h"
#include "UIConverter.h"
#include "UIIconPool.h"
#include "UIMachineSettingsNetwork.h"
#include "UIErrorString.h"
#include "UIExtraDataManager.h"
#include "UICommon.h"

/* COM includes: */
#include "CNATEngine.h"
#include "CNetworkAdapter.h"

/* Other VPox includes: */
#ifdef VPOX_WITH_VDE
# include <iprt/ldr.h>
# include <VPox/VDEPlug.h>
#endif


QString wipedOutString(const QString &strInputString)
{
    return strInputString.isEmpty() ? QString() : strInputString;
}


/** Machine settings: Network Adapter data structure. */
struct UIDataSettingsMachineNetworkAdapter
{
    /** Constructs data. */
    UIDataSettingsMachineNetworkAdapter()
        : m_iSlot(0)
        , m_fAdapterEnabled(false)
        , m_adapterType(KNetworkAdapterType_Null)
        , m_attachmentType(KNetworkAttachmentType_Null)
        , m_promiscuousMode(KNetworkAdapterPromiscModePolicy_Deny)
        , m_strBridgedAdapterName(QString())
        , m_strInternalNetworkName(QString())
        , m_strHostInterfaceName(QString())
        , m_strGenericDriverName(QString())
        , m_strGenericProperties(QString())
        , m_strNATNetworkName(QString())
#ifdef VPOX_WITH_CLOUD_NET
        , m_strCloudNetworkName(QString())
#endif /* VPOX_WITH_CLOUD_NET */
        , m_strMACAddress(QString())
        , m_fCableConnected(false)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineNetworkAdapter &other) const
    {
        return true
               && (m_iSlot == other.m_iSlot)
               && (m_fAdapterEnabled == other.m_fAdapterEnabled)
               && (m_adapterType == other.m_adapterType)
               && (m_attachmentType == other.m_attachmentType)
               && (m_promiscuousMode == other.m_promiscuousMode)
               && (m_strBridgedAdapterName == other.m_strBridgedAdapterName)
               && (m_strInternalNetworkName == other.m_strInternalNetworkName)
               && (m_strHostInterfaceName == other.m_strHostInterfaceName)
               && (m_strGenericDriverName == other.m_strGenericDriverName)
               && (m_strGenericProperties == other.m_strGenericProperties)
               && (m_strNATNetworkName == other.m_strNATNetworkName)
#ifdef VPOX_WITH_CLOUD_NET
               && (m_strCloudNetworkName == other.m_strCloudNetworkName)
#endif /* VPOX_WITH_CLOUD_NET */
               && (m_strMACAddress == other.m_strMACAddress)
               && (m_fCableConnected == other.m_fCableConnected)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineNetworkAdapter &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineNetworkAdapter &other) const { return !equal(other); }

    /** Holds the network adapter slot number. */
    int                               m_iSlot;
    /** Holds whether the network adapter is enabled. */
    bool                              m_fAdapterEnabled;
    /** Holds the network adapter type. */
    KNetworkAdapterType               m_adapterType;
    /** Holds the network attachment type. */
    KNetworkAttachmentType            m_attachmentType;
    /** Holds the network promiscuous mode policy. */
    KNetworkAdapterPromiscModePolicy  m_promiscuousMode;
    /** Holds the bridged adapter name. */
    QString                           m_strBridgedAdapterName;
    /** Holds the internal network name. */
    QString                           m_strInternalNetworkName;
    /** Holds the host interface name. */
    QString                           m_strHostInterfaceName;
    /** Holds the generic driver name. */
    QString                           m_strGenericDriverName;
    /** Holds the generic driver properties. */
    QString                           m_strGenericProperties;
    /** Holds the NAT network name. */
    QString                           m_strNATNetworkName;
#ifdef VPOX_WITH_CLOUD_NET
    /** Holds the cloud network name. */
    QString                           m_strCloudNetworkName;
#endif /* VPOX_WITH_CLOUD_NET */
    /** Holds the network adapter MAC address. */
    QString                           m_strMACAddress;
    /** Holds whether the network adapter is connected. */
    bool                              m_fCableConnected;
};


/** Machine settings: Network page data structure. */
struct UIDataSettingsMachineNetwork
{
    /** Constructs data. */
    UIDataSettingsMachineNetwork() {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineNetwork & /* other */) const { return true; }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineNetwork & /* other */) const { return false; }
};


/** Machine settings: Network Adapter tab. */
class UIMachineSettingsNetwork : public QIWithRetranslateUI<QWidget>,
                                 public Ui::UIMachineSettingsNetwork
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIMachineSettingsNetwork(UIMachineSettingsNetworkPage *pParent);

    /* Load / Save API: */
    void getAdapterDataFromCache(const UISettingsCacheMachineNetworkAdapter &adapterCache);
    void putAdapterDataToCache(UISettingsCacheMachineNetworkAdapter &adapterCache);

    /** Performs validation, updates @a messages list if something is wrong. */
    bool validate(QList<UIValidationMessage> &messages);

    /* Navigation stuff: */
    QWidget *setOrderAfter(QWidget *pAfter);

    /* Other public stuff: */
    QString tabTitle() const;
    KNetworkAttachmentType attachmentType() const;
    QString alternativeName(KNetworkAttachmentType enmType = KNetworkAttachmentType_Null) const;
    void polishTab();
    void reloadAlternatives();

    /** Defines whether the advanced button is @a fExpanded. */
    void setAdvancedButtonState(bool fExpanded);

signals:

    /* Signal to notify listeners about tab content changed: */
    void sigTabUpdated();

    /** Notifies about the advanced button has @a fExpanded. */
    void sigNotifyAdvancedButtonStateChange(bool fExpanded);

protected:

    /** Handles translation event. */
    void retranslateUi();

private slots:

    /* Different handlers: */
    void sltHandleAdapterActivityChange();
    void sltHandleAttachmentTypeChange();
    void sltHandleAlternativeNameChange();
    void sltHandleAdvancedButtonStateChange();
    void sltGenerateMac();
    void sltOpenPortForwardingDlg();

private:

    /* Helper: Prepare stuff: */
    void prepareValidation();

    /* Helping stuff: */
    void populateComboboxes();

    /** Handles advanced button state change. */
    void handleAdvancedButtonStateChange();

    /* Various static stuff: */
    static int position(QComboBox *pComboBox, int iData);
    static int position(QComboBox *pComboBox, const QString &strText);

    /* Parent page: */
    UIMachineSettingsNetworkPage *m_pParent;

    /* Other variables: */
    int m_iSlot;
    KNetworkAdapterType m_enmAdapterType;
    UIPortForwardingDataList m_portForwardingRules;
};


/*********************************************************************************************************************************
*   Class UIMachineSettingsNetwork implementation.                                                                               *
*********************************************************************************************************************************/

UIMachineSettingsNetwork::UIMachineSettingsNetwork(UIMachineSettingsNetworkPage *pParent)
    : QIWithRetranslateUI<QWidget>(0)
    , m_pParent(pParent)
    , m_iSlot(-1)
    , m_enmAdapterType(KNetworkAdapterType_Null)
{
    /* Apply UI decorations: */
    Ui::UIMachineSettingsNetwork::setupUi(this);

    /* Determine icon metric: */
    const QStyle *pStyle = QApplication::style();
    const int iIconMetric = (int)(pStyle->pixelMetric(QStyle::PM_SmallIconSize) * .625);

    /* Setup widgets: */
    m_pAttachmentTypeLabel->setBuddy(m_pAttachmentTypeEditor->focusProxy1());
    m_pAdapterNameLabel->setBuddy(m_pAttachmentTypeEditor->focusProxy2());
    m_pMACEditor->setValidator(new QRegExpValidator(QRegExp("[0-9A-Fa-f]{12}"), this));
    m_pMACEditor->setMinimumWidthByText(QString().fill('0', 12));
    m_pMACButton->setIcon(UIIconPool::iconSet(":/refresh_16px.png"));
    m_pAdvancedArrow->setIconSize(QSize(iIconMetric, iIconMetric));
    m_pAdvancedArrow->setIcons(UIIconPool::iconSet(":/arrow_right_10px.png"),
                               UIIconPool::iconSet(":/arrow_down_10px.png"));

    /* Setup connections: */
    connect(m_pEnableAdapterCheckBox, &QCheckBox::toggled, this, &UIMachineSettingsNetwork::sltHandleAdapterActivityChange);
    connect(m_pAttachmentTypeEditor, &UINetworkAttachmentEditor::sigValueTypeChanged,
            this, &UIMachineSettingsNetwork::sltHandleAttachmentTypeChange);
    connect(m_pAttachmentTypeEditor, &UINetworkAttachmentEditor::sigValueNameChanged,
            this, &UIMachineSettingsNetwork::sltHandleAlternativeNameChange);
    connect(m_pAdvancedArrow, &QIArrowButtonSwitch::sigClicked, this, &UIMachineSettingsNetwork::sltHandleAdvancedButtonStateChange);
    connect(m_pMACButton, &QIToolButton::clicked, this, &UIMachineSettingsNetwork::sltGenerateMac);
    connect(m_pPortForwardingButton, &QPushButton::clicked, this, &UIMachineSettingsNetwork::sltOpenPortForwardingDlg);
    connect(this, &UIMachineSettingsNetwork::sigTabUpdated, m_pParent, &UIMachineSettingsNetworkPage::sltHandleTabUpdate);

    /* Prepare validation: */
    prepareValidation();

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsNetwork::getAdapterDataFromCache(const UISettingsCacheMachineNetworkAdapter &adapterCache)
{
    /* Get old adapter data: */
    const UIDataSettingsMachineNetworkAdapter &oldAdapterData = adapterCache.base();

    /* Load slot number: */
    m_iSlot = oldAdapterData.m_iSlot;

    /* Load adapter activity state: */
    m_pEnableAdapterCheckBox->setChecked(oldAdapterData.m_fAdapterEnabled);
    /* Handle adapter activity change: */
    sltHandleAdapterActivityChange();

    /* Load attachment type: */
    m_pAttachmentTypeEditor->setValueType(oldAdapterData.m_attachmentType);
    /* Load alternative names: */
    m_pAttachmentTypeEditor->setValueName(KNetworkAttachmentType_Bridged, wipedOutString(oldAdapterData.m_strBridgedAdapterName));
    m_pAttachmentTypeEditor->setValueName(KNetworkAttachmentType_Internal, wipedOutString(oldAdapterData.m_strInternalNetworkName));
    m_pAttachmentTypeEditor->setValueName(KNetworkAttachmentType_HostOnly, wipedOutString(oldAdapterData.m_strHostInterfaceName));
    m_pAttachmentTypeEditor->setValueName(KNetworkAttachmentType_Generic, wipedOutString(oldAdapterData.m_strGenericDriverName));
    m_pAttachmentTypeEditor->setValueName(KNetworkAttachmentType_NATNetwork, wipedOutString(oldAdapterData.m_strNATNetworkName));
#ifdef VPOX_WITH_CLOUD_NET
    m_pAttachmentTypeEditor->setValueName(KNetworkAttachmentType_Cloud, wipedOutString(oldAdapterData.m_strCloudNetworkName));
#endif /* VPOX_WITH_CLOUD_NET */
    /* Handle attachment type change: */
    sltHandleAttachmentTypeChange();

    /* Load adapter type: */
    m_enmAdapterType = oldAdapterData.m_adapterType;

    /* Load promiscuous mode type: */
    m_pPromiscuousModeCombo->setCurrentIndex(position(m_pPromiscuousModeCombo, oldAdapterData.m_promiscuousMode));

    /* Other options: */
    m_pMACEditor->setText(oldAdapterData.m_strMACAddress);
    m_pGenericPropertiesTextEdit->setText(oldAdapterData.m_strGenericProperties);
    m_pCableConnectedCheckBox->setChecked(oldAdapterData.m_fCableConnected);

    /* Load port forwarding rules: */
    m_portForwardingRules.clear();
    for (int i = 0; i < adapterCache.childCount(); ++i)
        m_portForwardingRules << adapterCache.child(i).base();

    /* Repopulate combo-boxes content: */
    populateComboboxes();
    /* Reapply attachment info: */
    sltHandleAttachmentTypeChange();
}

void UIMachineSettingsNetwork::putAdapterDataToCache(UISettingsCacheMachineNetworkAdapter &adapterCache)
{
    /* Prepare new adapter data: */
    UIDataSettingsMachineNetworkAdapter newAdapterData;

    /* Save adapter activity state: */
    newAdapterData.m_fAdapterEnabled = m_pEnableAdapterCheckBox->isChecked();

    /* Save attachment type & alternative name: */
    newAdapterData.m_attachmentType = attachmentType();
    switch (newAdapterData.m_attachmentType)
    {
        case KNetworkAttachmentType_Null:
            break;
        case KNetworkAttachmentType_NAT:
            break;
        case KNetworkAttachmentType_Bridged:
            newAdapterData.m_strBridgedAdapterName = alternativeName();
            break;
        case KNetworkAttachmentType_Internal:
            newAdapterData.m_strInternalNetworkName = m_pAttachmentTypeEditor->valueName(KNetworkAttachmentType_Internal);
            break;
        case KNetworkAttachmentType_HostOnly:
            newAdapterData.m_strHostInterfaceName = m_pAttachmentTypeEditor->valueName(KNetworkAttachmentType_HostOnly);
            break;
        case KNetworkAttachmentType_Generic:
            newAdapterData.m_strGenericDriverName = m_pAttachmentTypeEditor->valueName(KNetworkAttachmentType_Generic);
            newAdapterData.m_strGenericProperties = m_pGenericPropertiesTextEdit->toPlainText();
            break;
        case KNetworkAttachmentType_NATNetwork:
            newAdapterData.m_strNATNetworkName = m_pAttachmentTypeEditor->valueName(KNetworkAttachmentType_NATNetwork);
            break;
#ifdef VPOX_WITH_CLOUD_NET
        case KNetworkAttachmentType_Cloud:
            newAdapterData.m_strCloudNetworkName = m_pAttachmentTypeEditor->valueName(KNetworkAttachmentType_Cloud);
            break;
#endif /* VPOX_WITH_CLOUD_NET */
        default:
            break;
    }

    /* Save adapter type: */
    newAdapterData.m_adapterType = m_pAdapterTypeCombo->currentData().value<KNetworkAdapterType>();

    /* Save promiscuous mode type: */
    newAdapterData.m_promiscuousMode = (KNetworkAdapterPromiscModePolicy)m_pPromiscuousModeCombo->itemData(m_pPromiscuousModeCombo->currentIndex()).toInt();

    /* Other options: */
    newAdapterData.m_strMACAddress = m_pMACEditor->text().isEmpty() ? QString() : m_pMACEditor->text();
    newAdapterData.m_fCableConnected = m_pCableConnectedCheckBox->isChecked();

    /* Save port forwarding rules: */
    foreach (const UIDataPortForwardingRule &rule, m_portForwardingRules)
        adapterCache.child(rule.name).cacheCurrentData(rule);

    /* Cache new adapter data: */
    adapterCache.cacheCurrentData(newAdapterData);
}

bool UIMachineSettingsNetwork::validate(QList<UIValidationMessage> &messages)
{
    /* Pass if adapter is disabled: */
    if (!m_pEnableAdapterCheckBox->isChecked())
        return true;

    /* Pass by default: */
    bool fPass = true;

    /* Prepare message: */
    UIValidationMessage message;
    message.first = uiCommon().removeAccelMark(tabTitle());

    /* Validate alternatives: */
    switch (attachmentType())
    {
        case KNetworkAttachmentType_Bridged:
        {
            if (alternativeName().isNull())
            {
                message.second << tr("No bridged network adapter is currently selected.");
                fPass = false;
            }
            break;
        }
        case KNetworkAttachmentType_Internal:
        {
            if (alternativeName().isNull())
            {
                message.second << tr("No internal network name is currently specified.");
                fPass = false;
            }
            break;
        }
        case KNetworkAttachmentType_HostOnly:
        {
            if (alternativeName().isNull())
            {
                message.second << tr("No host-only network adapter is currently selected.");
                fPass = false;
            }
            break;
        }
        case KNetworkAttachmentType_Generic:
        {
            if (alternativeName().isNull())
            {
                message.second << tr("No generic driver is currently selected.");
                fPass = false;
            }
            break;
        }
        case KNetworkAttachmentType_NATNetwork:
        {
            if (alternativeName().isNull())
            {
                message.second << tr("No NAT network name is currently specified.");
                fPass = false;
            }
            break;
        }
#ifdef VPOX_WITH_CLOUD_NET
        case KNetworkAttachmentType_Cloud:
        {
            if (alternativeName().isNull())
            {
                message.second << tr("No cloud network name is currently specified.");
                fPass = false;
            }
            break;
        }
#endif /* VPOX_WITH_CLOUD_NET */
        default:
            break;
    }

    /* Validate MAC-address length: */
    if (m_pMACEditor->text().size() < 12)
    {
        message.second << tr("The MAC address must be 12 hexadecimal digits long.");
        fPass = false;
    }

    /* Make sure MAC-address is unicast: */
    if (m_pMACEditor->text().size() >= 2)
    {
        QRegExp validator("^[0-9A-Fa-f][02468ACEace]");
        if (validator.indexIn(m_pMACEditor->text()) != 0)
        {
            message.second << tr("The second digit in the MAC address may not be odd as only unicast addresses are allowed.");
            fPass = false;
        }
    }

    /* Serialize message: */
    if (!message.second.isEmpty())
        messages << message;

    /* Return result: */
    return fPass;
}

QWidget *UIMachineSettingsNetwork::setOrderAfter(QWidget *pAfter)
{
    setTabOrder(pAfter, m_pEnableAdapterCheckBox);
    setTabOrder(m_pEnableAdapterCheckBox, m_pAttachmentTypeEditor);
    setTabOrder(m_pAttachmentTypeEditor, m_pAdvancedArrow);
    setTabOrder(m_pAdvancedArrow, m_pAdapterTypeCombo);
    setTabOrder(m_pAdapterTypeCombo, m_pPromiscuousModeCombo);
    setTabOrder(m_pPromiscuousModeCombo, m_pMACEditor);
    setTabOrder(m_pMACEditor, m_pMACButton);
    setTabOrder(m_pMACButton, m_pGenericPropertiesTextEdit);
    setTabOrder(m_pGenericPropertiesTextEdit, m_pCableConnectedCheckBox);
    setTabOrder(m_pCableConnectedCheckBox, m_pPortForwardingButton);
    return m_pPortForwardingButton;
}

QString UIMachineSettingsNetwork::tabTitle() const
{
    return UICommon::tr("Adapter %1").arg(QString("&%1").arg(m_iSlot + 1));
}

KNetworkAttachmentType UIMachineSettingsNetwork::attachmentType() const
{
    return m_pAttachmentTypeEditor->valueType();
}

QString UIMachineSettingsNetwork::alternativeName(KNetworkAttachmentType enmType /* = KNetworkAttachmentType_Null */) const
{
    if (enmType == KNetworkAttachmentType_Null)
        enmType = attachmentType();
    return m_pAttachmentTypeEditor->valueName(enmType);
}

void UIMachineSettingsNetwork::polishTab()
{
    /* Basic attributes: */
    m_pEnableAdapterCheckBox->setEnabled(m_pParent->isMachineOffline());
    m_pAttachmentTypeLabel->setEnabled(m_pParent->isMachineInValidMode());
    m_pAttachmentTypeEditor->setEnabled(m_pParent->isMachineInValidMode());
    m_pAdapterNameLabel->setEnabled(m_pParent->isMachineInValidMode() &&
                                    attachmentType() != KNetworkAttachmentType_Null &&
                                    attachmentType() != KNetworkAttachmentType_NAT);
    m_pAdvancedArrow->setEnabled(m_pParent->isMachineInValidMode());

    /* Advanced attributes: */
    m_pAdapterTypeLabel->setEnabled(m_pParent->isMachineOffline());
    m_pAdapterTypeCombo->setEnabled(m_pParent->isMachineOffline());
    m_pPromiscuousModeLabel->setEnabled(m_pParent->isMachineInValidMode() &&
                                        attachmentType() != KNetworkAttachmentType_Null &&
                                        attachmentType() != KNetworkAttachmentType_Generic &&
                                        attachmentType() != KNetworkAttachmentType_NAT);
    m_pPromiscuousModeCombo->setEnabled(m_pParent->isMachineInValidMode() &&
                                        attachmentType() != KNetworkAttachmentType_Null &&
                                        attachmentType() != KNetworkAttachmentType_Generic &&
                                        attachmentType() != KNetworkAttachmentType_NAT);
    m_pMACLabel->setEnabled(m_pParent->isMachineOffline());
    m_pMACEditor->setEnabled(m_pParent->isMachineOffline());
    m_pMACButton->setEnabled(m_pParent->isMachineOffline());
    m_pGenericPropertiesLabel->setEnabled(m_pParent->isMachineInValidMode());
    m_pGenericPropertiesTextEdit->setEnabled(m_pParent->isMachineInValidMode());
    m_pCableConnectedCheckBox->setEnabled(m_pParent->isMachineInValidMode());
    m_pPortForwardingButton->setEnabled(m_pParent->isMachineInValidMode() &&
                                        attachmentType() == KNetworkAttachmentType_NAT);

    /* Postprocessing: */
    handleAdvancedButtonStateChange();
}

void UIMachineSettingsNetwork::reloadAlternatives()
{
    m_pAttachmentTypeEditor->setValueNames(KNetworkAttachmentType_Bridged, m_pParent->bridgedAdapterList());
    m_pAttachmentTypeEditor->setValueNames(KNetworkAttachmentType_Internal, m_pParent->internalNetworkList());
    m_pAttachmentTypeEditor->setValueNames(KNetworkAttachmentType_HostOnly, m_pParent->hostInterfaceList());
    m_pAttachmentTypeEditor->setValueNames(KNetworkAttachmentType_Generic, m_pParent->genericDriverList());
    m_pAttachmentTypeEditor->setValueNames(KNetworkAttachmentType_NATNetwork, m_pParent->natNetworkList());
#ifdef VPOX_WITH_CLOUD_NET
    m_pAttachmentTypeEditor->setValueNames(KNetworkAttachmentType_Cloud, m_pParent->cloudNetworkList());
#endif /* VPOX_WITH_CLOUD_NET */
}

void UIMachineSettingsNetwork::setAdvancedButtonState(bool fExpanded)
{
    /* Check whether the button state really changed: */
    if (m_pAdvancedArrow->isExpanded() == fExpanded)
        return;

    /* Push the state to button and handle the state change: */
    m_pAdvancedArrow->setExpanded(fExpanded);
    handleAdvancedButtonStateChange();
}

void UIMachineSettingsNetwork::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIMachineSettingsNetwork::retranslateUi(this);

    /* Translate combo-boxes content: */
    populateComboboxes();

    /* Translate attachment info: */
    sltHandleAttachmentTypeChange();
}

void UIMachineSettingsNetwork::sltHandleAdapterActivityChange()
{
    /* Update availability: */
    m_pAdapterOptionsContainer->setEnabled(m_pEnableAdapterCheckBox->isChecked());

    /* Generate a new MAC address in case this adapter was never enabled before: */
    if (   m_pEnableAdapterCheckBox->isChecked()
        && m_pMACEditor->text().isEmpty())
        sltGenerateMac();

    /* Revalidate: */
    m_pParent->revalidate();
}

void UIMachineSettingsNetwork::sltHandleAttachmentTypeChange()
{
    /* Update alternative-name combo-box availability: */
    m_pAdapterNameLabel->setEnabled(attachmentType() != KNetworkAttachmentType_Null &&
                                    attachmentType() != KNetworkAttachmentType_NAT);
    /* Update promiscuous-mode combo-box availability: */
    m_pPromiscuousModeLabel->setEnabled(attachmentType() != KNetworkAttachmentType_Null &&
                                        attachmentType() != KNetworkAttachmentType_Generic &&
                                        attachmentType() != KNetworkAttachmentType_NAT);
    m_pPromiscuousModeCombo->setEnabled(attachmentType() != KNetworkAttachmentType_Null &&
                                        attachmentType() != KNetworkAttachmentType_Generic &&
                                        attachmentType() != KNetworkAttachmentType_NAT);
    /* Update generic-properties editor visibility: */
    m_pGenericPropertiesLabel->setVisible(attachmentType() == KNetworkAttachmentType_Generic &&
                                          m_pAdvancedArrow->isExpanded());
    m_pGenericPropertiesTextEdit->setVisible(attachmentType() == KNetworkAttachmentType_Generic &&
                                             m_pAdvancedArrow->isExpanded());
    /* Update forwarding rules button availability: */
    m_pPortForwardingButton->setEnabled(attachmentType() == KNetworkAttachmentType_NAT);

    /* Revalidate: */
    m_pParent->revalidate();
}

void UIMachineSettingsNetwork::sltHandleAlternativeNameChange()
{
    /* Remember new name if its changed,
     * Call for other pages update, if necessary: */
    switch (attachmentType())
    {
        case KNetworkAttachmentType_Internal:
        {
            if (!m_pAttachmentTypeEditor->valueName(KNetworkAttachmentType_Internal).isNull())
                emit sigTabUpdated();
            break;
        }
        case KNetworkAttachmentType_Generic:
        {
            if (!m_pAttachmentTypeEditor->valueName(KNetworkAttachmentType_Generic).isNull())
                emit sigTabUpdated();
            break;
        }
        default:
            break;
    }

    /* Revalidate: */
    m_pParent->revalidate();
}

void UIMachineSettingsNetwork::sltHandleAdvancedButtonStateChange()
{
    /* Handle the button state change: */
    handleAdvancedButtonStateChange();

    /* Notify listeners about the button state change: */
    emit sigNotifyAdvancedButtonStateChange(m_pAdvancedArrow->isExpanded());
}

void UIMachineSettingsNetwork::sltGenerateMac()
{
    m_pMACEditor->setText(uiCommon().host().GenerateMACAddress());
}

void UIMachineSettingsNetwork::sltOpenPortForwardingDlg()
{
    UIMachineSettingsPortForwardingDlg dlg(this, m_portForwardingRules);
    if (dlg.exec() == QDialog::Accepted)
        m_portForwardingRules = dlg.rules();
}

void UIMachineSettingsNetwork::prepareValidation()
{
    /* Configure validation: */
    connect(m_pMACEditor, &QILineEdit::textChanged, m_pParent, &UIMachineSettingsNetworkPage::revalidate);
}

void UIMachineSettingsNetwork::populateComboboxes()
{
    /* Adapter names: */
    {
        reloadAlternatives();
    }

    /* Adapter type: */
    {
        /* Clear the adapter type combo-box: */
        m_pAdapterTypeCombo->clear();

        /* Load currently supported network adapter types: */
        CSystemProperties comProperties = uiCommon().virtualPox().GetSystemProperties();
        QVector<KNetworkAdapterType> supportedTypes = comProperties.GetSupportedNetworkAdapterTypes();
        /* Take currently requested type into account if it's sane: */
        if (!supportedTypes.contains(m_enmAdapterType) && m_enmAdapterType != KNetworkAdapterType_Null)
            supportedTypes.prepend(m_enmAdapterType);

        /* Populate adapter types: */
        int iAdapterTypeIndex = 0;
        foreach (const KNetworkAdapterType &enmType, supportedTypes)
        {
            m_pAdapterTypeCombo->insertItem(iAdapterTypeIndex, gpConverter->toString(enmType));
            m_pAdapterTypeCombo->setItemData(iAdapterTypeIndex, QVariant::fromValue(enmType));
            m_pAdapterTypeCombo->setItemData(iAdapterTypeIndex, m_pAdapterTypeCombo->itemText(iAdapterTypeIndex), Qt::ToolTipRole);
            ++iAdapterTypeIndex;
        }

        /* Choose requested adapter type: */
        const int iIndex = m_pAdapterTypeCombo->findData(m_enmAdapterType);
        m_pAdapterTypeCombo->setCurrentIndex(iIndex != -1 ? iIndex : 0);
    }

    /* Promiscuous Mode type: */
    {
        /* Remember the currently selected promiscuous mode type: */
        int iCurrentPromiscuousMode = m_pPromiscuousModeCombo->currentIndex();

        /* Clear the promiscuous mode combo-box: */
        m_pPromiscuousModeCombo->clear();

        /* Populate promiscuous modes: */
        int iPromiscuousModeIndex = 0;
        m_pPromiscuousModeCombo->insertItem(iPromiscuousModeIndex, gpConverter->toString(KNetworkAdapterPromiscModePolicy_Deny));
        m_pPromiscuousModeCombo->setItemData(iPromiscuousModeIndex, KNetworkAdapterPromiscModePolicy_Deny);
        m_pPromiscuousModeCombo->setItemData(iPromiscuousModeIndex, m_pPromiscuousModeCombo->itemText(iPromiscuousModeIndex), Qt::ToolTipRole);
        ++iPromiscuousModeIndex;
        m_pPromiscuousModeCombo->insertItem(iPromiscuousModeIndex, gpConverter->toString(KNetworkAdapterPromiscModePolicy_AllowNetwork));
        m_pPromiscuousModeCombo->setItemData(iPromiscuousModeIndex, KNetworkAdapterPromiscModePolicy_AllowNetwork);
        m_pPromiscuousModeCombo->setItemData(iPromiscuousModeIndex, m_pPromiscuousModeCombo->itemText(iPromiscuousModeIndex), Qt::ToolTipRole);
        ++iPromiscuousModeIndex;
        m_pPromiscuousModeCombo->insertItem(iPromiscuousModeIndex, gpConverter->toString(KNetworkAdapterPromiscModePolicy_AllowAll));
        m_pPromiscuousModeCombo->setItemData(iPromiscuousModeIndex, KNetworkAdapterPromiscModePolicy_AllowAll);
        m_pPromiscuousModeCombo->setItemData(iPromiscuousModeIndex, m_pPromiscuousModeCombo->itemText(iPromiscuousModeIndex), Qt::ToolTipRole);
        ++iPromiscuousModeIndex;

        /* Restore the previously selected promiscuous mode type: */
        m_pPromiscuousModeCombo->setCurrentIndex(iCurrentPromiscuousMode == -1 ? 0 : iCurrentPromiscuousMode);
    }
}

void UIMachineSettingsNetwork::handleAdvancedButtonStateChange()
{
    /* Update visibility of advanced options: */
    m_pAdapterTypeLabel->setVisible(m_pAdvancedArrow->isExpanded());
    m_pAdapterTypeCombo->setVisible(m_pAdvancedArrow->isExpanded());
    m_pPromiscuousModeLabel->setVisible(m_pAdvancedArrow->isExpanded());
    m_pPromiscuousModeCombo->setVisible(m_pAdvancedArrow->isExpanded());
    m_pGenericPropertiesLabel->setVisible(attachmentType() == KNetworkAttachmentType_Generic &&
                                          m_pAdvancedArrow->isExpanded());
    m_pGenericPropertiesTextEdit->setVisible(attachmentType() == KNetworkAttachmentType_Generic &&
                                             m_pAdvancedArrow->isExpanded());
    m_pMACLabel->setVisible(m_pAdvancedArrow->isExpanded());
    m_pMACEditor->setVisible(m_pAdvancedArrow->isExpanded());
    m_pMACButton->setVisible(m_pAdvancedArrow->isExpanded());
    m_pCableConnectedCheckBox->setVisible(m_pAdvancedArrow->isExpanded());
    m_pPortForwardingButton->setVisible(m_pAdvancedArrow->isExpanded());
}

/* static */
int UIMachineSettingsNetwork::position(QComboBox *pComboBox, int iData)
{
    const int iPosition = pComboBox->findData(iData);
    return iPosition == -1 ? 0 : iPosition;
}

/* static */
int UIMachineSettingsNetwork::position(QComboBox *pComboBox, const QString &strText)
{
    const int iPosition = pComboBox->findText(strText);
    return iPosition == -1 ? 0 : iPosition;
}


/*********************************************************************************************************************************
*   Class UIMachineSettingsNetworkPage implementation.                                                                           *
*********************************************************************************************************************************/

UIMachineSettingsNetworkPage::UIMachineSettingsNetworkPage()
    : m_pTabWidget(0)
    , m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIMachineSettingsNetworkPage::~UIMachineSettingsNetworkPage()
{
    /* Cleanup: */
    cleanup();
}

bool UIMachineSettingsNetworkPage::changed() const
{
    return m_pCache->wasChanged();
}

void UIMachineSettingsNetworkPage::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Cache name lists: */
    refreshBridgedAdapterList();
    refreshInternalNetworkList(true);
    refreshHostInterfaceList();
    refreshGenericDriverList(true);
    refreshNATNetworkList();
#ifdef VPOX_WITH_CLOUD_NET
    refreshCloudNetworkList();
#endif /* VPOX_WITH_CLOUD_NET */

    /* Prepare old network data: */
    UIDataSettingsMachineNetwork oldNetworkData;

    /* For each network adapter: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Prepare old adapter data: */
        UIDataSettingsMachineNetworkAdapter oldAdapterData;

        /* Check whether adapter is valid: */
        const CNetworkAdapter &comAdapter = m_machine.GetNetworkAdapter(iSlot);
        if (!comAdapter.isNull())
        {
            /* Gather old adapter data: */
            oldAdapterData.m_iSlot = iSlot;
            oldAdapterData.m_fAdapterEnabled = comAdapter.GetEnabled();
            oldAdapterData.m_attachmentType = comAdapter.GetAttachmentType();
            oldAdapterData.m_strBridgedAdapterName = wipedOutString(comAdapter.GetBridgedInterface());
            oldAdapterData.m_strInternalNetworkName = wipedOutString(comAdapter.GetInternalNetwork());
            oldAdapterData.m_strHostInterfaceName = wipedOutString(comAdapter.GetHostOnlyInterface());
            oldAdapterData.m_strGenericDriverName = wipedOutString(comAdapter.GetGenericDriver());
            oldAdapterData.m_strNATNetworkName = wipedOutString(comAdapter.GetNATNetwork());
#ifdef VPOX_WITH_CLOUD_NET
            oldAdapterData.m_strCloudNetworkName = wipedOutString(comAdapter.GetCloudNetwork());
#endif /* VPOX_WITH_CLOUD_NET */
            oldAdapterData.m_adapterType = comAdapter.GetAdapterType();
            oldAdapterData.m_promiscuousMode = comAdapter.GetPromiscModePolicy();
            oldAdapterData.m_strMACAddress = comAdapter.GetMACAddress();
            oldAdapterData.m_strGenericProperties = loadGenericProperties(comAdapter);
            oldAdapterData.m_fCableConnected = comAdapter.GetCableConnected();
            foreach (const QString &strRedirect, comAdapter.GetNATEngine().GetRedirects())
            {
                /* Gather old forwarding data & cache key: */
                const QStringList &forwardingData = strRedirect.split(',');
                AssertMsg(forwardingData.size() == 6, ("Redirect rule should be composed of 6 parts!\n"));
                const UIDataPortForwardingRule oldForwardingData(forwardingData.at(0),
                                                                 (KNATProtocol)forwardingData.at(1).toUInt(),
                                                                 forwardingData.at(2),
                                                                 forwardingData.at(3).toUInt(),
                                                                 forwardingData.at(4),
                                                                 forwardingData.at(5).toUInt());

                const QString &strForwardingKey = forwardingData.at(0);
                /* Cache old forwarding data: */
                m_pCache->child(iSlot).child(strForwardingKey).cacheInitialData(oldForwardingData);
            }
        }

        /* Cache old adapter data: */
        m_pCache->child(iSlot).cacheInitialData(oldAdapterData);
    }

    /* Cache old network data: */
    m_pCache->cacheInitialData(oldNetworkData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsNetworkPage::getFromCache()
{
    /* Setup tab order: */
    AssertPtrReturnVoid(firstWidget());
    setTabOrder(firstWidget(), m_pTabWidget->focusProxy());
    QWidget *pLastFocusWidget = m_pTabWidget->focusProxy();

    /* For each adapter: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Get adapter page: */
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));

        /* Load old adapter data from the cache: */
        pTab->getAdapterDataFromCache(m_pCache->child(iSlot));

        /* Setup tab order: */
        pLastFocusWidget = pTab->setOrderAfter(pLastFocusWidget);
    }

    /* Apply language settings: */
    retranslateUi();

    /* Polish page finally: */
    polishPage();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsNetworkPage::putToCache()
{
    /* Prepare new network data: */
    UIDataSettingsMachineNetwork newNetworkData;

    /* For each adapter: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Get adapter page: */
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));

        /* Gather new adapter data: */
        pTab->putAdapterDataToCache(m_pCache->child(iSlot));
    }

    /* Cache new network data: */
    m_pCache->cacheCurrentData(newNetworkData);
}

void UIMachineSettingsNetworkPage::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update network data and failing state: */
    setFailed(!saveNetworkData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

bool UIMachineSettingsNetworkPage::validate(QList<UIValidationMessage> &messages)
{
    /* Pass by default: */
    bool fValid = true;

    /* Delegate validation to adapter tabs: */
    for (int i = 0; i < m_pTabWidget->count(); ++i)
    {
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(i));
        AssertMsg(pTab, ("Can't get adapter tab!\n"));
        if (!pTab->validate(messages))
            fValid = false;
    }

    /* Return result: */
    return fValid;
}

void UIMachineSettingsNetworkPage::retranslateUi()
{
    for (int i = 0; i < m_pTabWidget->count(); ++i)
    {
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(i));
        Assert(pTab);
        m_pTabWidget->setTabText(i, pTab->tabTitle());
    }
}

void UIMachineSettingsNetworkPage::polishPage()
{
    /* Get the count of network adapter tabs: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        m_pTabWidget->setTabEnabled(iSlot,
                                    isMachineOffline() ||
                                    (isMachineInValidMode() &&
                                     m_pCache->childCount() > iSlot &&
                                     m_pCache->child(iSlot).base().m_fAdapterEnabled));
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        pTab->polishTab();
    }
}

void UIMachineSettingsNetworkPage::sltHandleTabUpdate()
{
    /* Determine the sender: */
    UIMachineSettingsNetwork *pSender = qobject_cast<UIMachineSettingsNetwork*>(sender());
    AssertMsg(pSender, ("This slot should be called only through signal<->slot mechanism from one of UIMachineSettingsNetwork tabs!\n"));

    /* Determine sender's attachment type: */
    const KNetworkAttachmentType enmSenderAttachmentType = pSender->attachmentType();
    switch (enmSenderAttachmentType)
    {
        case KNetworkAttachmentType_Internal:
        {
            refreshInternalNetworkList();
            break;
        }
        case KNetworkAttachmentType_Generic:
        {
            refreshGenericDriverList();
            break;
        }
        default:
            break;
    }

    /* Update all the tabs except the sender: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Get the iterated tab: */
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        AssertMsg(pTab, ("All the tabs of m_pTabWidget should be of the UIMachineSettingsNetwork type!\n"));

        /* Update all the tabs (except sender): */
        if (pTab != pSender)
            pTab->reloadAlternatives();
    }
}

void UIMachineSettingsNetworkPage::sltHandleAdvancedButtonStateChange(bool fExpanded)
{
    /* Update the advanced button states for all the pages: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        pTab->setAdvancedButtonState(fExpanded);
    }
}

void UIMachineSettingsNetworkPage::prepare()
{
    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineNetwork;
    AssertPtrReturnVoid(m_pCache);

    /* Create main layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    AssertPtrReturnVoid(pMainLayout);
    {
        /* Creating tab-widget: */
        m_pTabWidget = new QITabWidget;
        AssertPtrReturnVoid(m_pTabWidget);
        {
            /* How many adapters to display: */
            /** @todo r=klaus this needs to be done based on the actual chipset type of the VM,
              * but in this place the m_machine field isn't set yet. My observation (on Linux)
              * is that the limitation to 4 isn't necessary any more, but this needs to be checked
              * on all platforms to be certain that it's usable everywhere. */
            const ulong uCount = qMin((ULONG)4, uiCommon().virtualPox().GetSystemProperties().GetMaxNetworkAdapters(KChipsetType_PIIX3));

            /* Create corresponding adapter tabs: */
            for (ulong uSlot = 0; uSlot < uCount; ++uSlot)
            {
                /* Create adapter tab: */
                UIMachineSettingsNetwork *pTab = new UIMachineSettingsNetwork(this);
                AssertPtrReturnVoid(pTab);
                {
                    /* Configure tab: */
                    connect(pTab, &UIMachineSettingsNetwork::sigNotifyAdvancedButtonStateChange,
                            this, &UIMachineSettingsNetworkPage::sltHandleAdvancedButtonStateChange);

                    /* Add tab into tab-widget: */
                    m_pTabWidget->addTab(pTab, pTab->tabTitle());
                }
            }

            /* Add tab-widget into layout: */
            pMainLayout->addWidget(m_pTabWidget);
        }
    }
}

void UIMachineSettingsNetworkPage::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

void UIMachineSettingsNetworkPage::refreshBridgedAdapterList()
{
    /* Reload bridged adapters: */
    m_bridgedAdapterList = UINetworkAttachmentEditor::bridgedAdapters();
}

void UIMachineSettingsNetworkPage::refreshInternalNetworkList(bool fFullRefresh /* = false */)
{
    /* Reload internal network list: */
    m_internalNetworkList.clear();
    /* Get internal network names from other VMs: */
    if (fFullRefresh)
        m_internalNetworkListSaved = UINetworkAttachmentEditor::internalNetworks();
    m_internalNetworkList << m_internalNetworkListSaved;
    /* Append internal network list with names from all the tabs: */
    for (int iTab = 0; iTab < m_pTabWidget->count(); ++iTab)
    {
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iTab));
        if (pTab)
        {
            const QString strName = pTab->alternativeName(KNetworkAttachmentType_Internal);
            if (!strName.isEmpty() && !m_internalNetworkList.contains(strName))
                m_internalNetworkList << strName;
        }
    }
}

#ifdef VPOX_WITH_CLOUD_NET
void UIMachineSettingsNetworkPage::refreshCloudNetworkList()
{
    /* Reload cloud network list: */
    m_cloudNetworkList = UINetworkAttachmentEditor::cloudNetworks();
}
#endif /* VPOX_WITH_CLOUD_NET */

void UIMachineSettingsNetworkPage::refreshHostInterfaceList()
{
    /* Reload host interfaces: */
    m_hostInterfaceList = UINetworkAttachmentEditor::hostInterfaces();
}

void UIMachineSettingsNetworkPage::refreshGenericDriverList(bool fFullRefresh /* = false */)
{
    /* Load generic driver list: */
    m_genericDriverList.clear();
    /* Get generic driver names from other VMs: */
    if (fFullRefresh)
        m_genericDriverListSaved = UINetworkAttachmentEditor::genericDrivers();
    m_genericDriverList << m_genericDriverListSaved;
    /* Append generic driver list with names from all the tabs: */
    for (int iTab = 0; iTab < m_pTabWidget->count(); ++iTab)
    {
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iTab));
        if (pTab)
        {
            const QString strName = pTab->alternativeName(KNetworkAttachmentType_Generic);
            if (!strName.isEmpty() && !m_genericDriverList.contains(strName))
                m_genericDriverList << strName;
        }
    }
}

void UIMachineSettingsNetworkPage::refreshNATNetworkList()
{
    /* Reload nat networks: */
    m_natNetworkList = UINetworkAttachmentEditor::natNetworks();
}

/* static */
QString UIMachineSettingsNetworkPage::loadGenericProperties(const CNetworkAdapter &adapter)
{
    /* Prepare formatted string: */
    QVector<QString> names;
    QVector<QString> props;
    props = adapter.GetProperties(QString(), names);
    QString strResult;
    /* Load generic properties: */
    for (int i = 0; i < names.size(); ++i)
    {
        strResult += names[i] + "=" + props[i];
        if (i < names.size() - 1)
          strResult += "\n";
    }
    /* Return formatted string: */
    return strResult;
}

/* static */
bool UIMachineSettingsNetworkPage::saveGenericProperties(CNetworkAdapter &comAdapter, const QString &strProperties)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save generic properties: */
    if (fSuccess)
    {
        /* Acquire 'added' properties: */
        const QStringList newProps = strProperties.split("\n");

        /* Insert 'added' properties: */
        QHash<QString, QString> hash;
        for (int i = 0; fSuccess && i < newProps.size(); ++i)
        {
            /* Parse property line: */
            const QString strLine = newProps.at(i);
            const QString strKey = strLine.section('=', 0, 0);
            const QString strVal = strLine.section('=', 1, -1);
            if (strKey.isEmpty() || strVal.isEmpty())
                continue;
            /* Save property in the adapter and the hash: */
            comAdapter.SetProperty(strKey, strVal);
            fSuccess = comAdapter.isOk();
            hash[strKey] = strVal;
        }

        /* Acquire actual properties ('added' and 'removed'): */
        QVector<QString> names;
        QVector<QString> props;
        if (fSuccess)
        {
            props = comAdapter.GetProperties(QString(), names);
            fSuccess = comAdapter.isOk();
        }

        /* Exclude 'removed' properties: */
        for (int i = 0; fSuccess && i < names.size(); ++i)
        {
            /* Get property name and value: */
            const QString strKey = names.at(i);
            const QString strVal = props.at(i);
            if (strVal == hash.value(strKey))
                continue;
            /* Remove property from the adapter: */
            // Actually we are _replacing_ property value,
            // not _removing_ it at all, but we are replacing it
            // with default constructed value, which is QString().
            comAdapter.SetProperty(strKey, hash.value(strKey));
            fSuccess = comAdapter.isOk();
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsNetworkPage::saveNetworkData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save network settings from the cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* For each adapter: */
        for (int iSlot = 0; fSuccess && iSlot < m_pTabWidget->count(); ++iSlot)
            fSuccess = saveAdapterData(iSlot);
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsNetworkPage::saveAdapterData(int iSlot)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save adapter settings from the cache: */
    if (fSuccess && m_pCache->child(iSlot).wasChanged())
    {
        /* Get old network data from the cache: */
        const UIDataSettingsMachineNetworkAdapter &oldAdapterData = m_pCache->child(iSlot).base();
        /* Get new network data from the cache: */
        const UIDataSettingsMachineNetworkAdapter &newAdapterData = m_pCache->child(iSlot).data();

        /* Get network adapter for further activities: */
        CNetworkAdapter comAdapter = m_machine.GetNetworkAdapter(iSlot);
        fSuccess = m_machine.isOk() && comAdapter.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* Save whether the adapter is enabled: */
            if (fSuccess && isMachineOffline() && newAdapterData.m_fAdapterEnabled != oldAdapterData.m_fAdapterEnabled)
            {
                comAdapter.SetEnabled(newAdapterData.m_fAdapterEnabled);
                fSuccess = comAdapter.isOk();
            }
            /* Save adapter type: */
            if (fSuccess && isMachineOffline() && newAdapterData.m_adapterType != oldAdapterData.m_adapterType)
            {
                comAdapter.SetAdapterType(newAdapterData.m_adapterType);
                fSuccess = comAdapter.isOk();
            }
            /* Save adapter MAC address: */
            if (fSuccess && isMachineOffline() && newAdapterData.m_strMACAddress != oldAdapterData.m_strMACAddress)
            {
                comAdapter.SetMACAddress(newAdapterData.m_strMACAddress);
                fSuccess = comAdapter.isOk();
            }
            /* Save adapter attachment type: */
            switch (newAdapterData.m_attachmentType)
            {
                case KNetworkAttachmentType_Bridged:
                {
                    if (fSuccess && newAdapterData.m_strBridgedAdapterName != oldAdapterData.m_strBridgedAdapterName)
                    {
                        comAdapter.SetBridgedInterface(newAdapterData.m_strBridgedAdapterName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
                case KNetworkAttachmentType_Internal:
                {
                    if (fSuccess && newAdapterData.m_strInternalNetworkName != oldAdapterData.m_strInternalNetworkName)
                    {
                        comAdapter.SetInternalNetwork(newAdapterData.m_strInternalNetworkName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
                case KNetworkAttachmentType_HostOnly:
                {
                    if (fSuccess && newAdapterData.m_strHostInterfaceName != oldAdapterData.m_strHostInterfaceName)
                    {
                        comAdapter.SetHostOnlyInterface(newAdapterData.m_strHostInterfaceName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
                case KNetworkAttachmentType_Generic:
                {
                    if (fSuccess && newAdapterData.m_strGenericDriverName != oldAdapterData.m_strGenericDriverName)
                    {
                        comAdapter.SetGenericDriver(newAdapterData.m_strGenericDriverName);
                        fSuccess = comAdapter.isOk();
                    }
                    if (fSuccess && newAdapterData.m_strGenericProperties != oldAdapterData.m_strGenericProperties)
                        fSuccess = saveGenericProperties(comAdapter, newAdapterData.m_strGenericProperties);
                    break;
                }
                case KNetworkAttachmentType_NATNetwork:
                {
                    if (fSuccess && newAdapterData.m_strNATNetworkName != oldAdapterData.m_strNATNetworkName)
                    {
                        comAdapter.SetNATNetwork(newAdapterData.m_strNATNetworkName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
#ifdef VPOX_WITH_CLOUD_NET
                case KNetworkAttachmentType_Cloud:
                {
                    if (fSuccess && newAdapterData.m_strCloudNetworkName != oldAdapterData.m_strCloudNetworkName)
                    {
                        comAdapter.SetCloudNetwork(newAdapterData.m_strCloudNetworkName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
#endif /* VPOX_WITH_CLOUD_NET */
                default:
                    break;
            }
            if (fSuccess && newAdapterData.m_attachmentType != oldAdapterData.m_attachmentType)
            {
                comAdapter.SetAttachmentType(newAdapterData.m_attachmentType);
                fSuccess = comAdapter.isOk();
            }
            /* Save adapter promiscuous mode: */
            if (fSuccess && newAdapterData.m_promiscuousMode != oldAdapterData.m_promiscuousMode)
            {
                comAdapter.SetPromiscModePolicy(newAdapterData.m_promiscuousMode);
                fSuccess = comAdapter.isOk();
            }
            /* Save whether the adapter cable connected: */
            if (fSuccess && newAdapterData.m_fCableConnected != oldAdapterData.m_fCableConnected)
            {
                comAdapter.SetCableConnected(newAdapterData.m_fCableConnected);
                fSuccess = comAdapter.isOk();
            }

            /* Get NAT engine for further activities: */
            CNATEngine comEngine;
            if (fSuccess)
            {
                comEngine = comAdapter.GetNATEngine();
                fSuccess = comAdapter.isOk() && comEngine.isNotNull();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comAdapter));
            else
            {
                /* Save adapter port forwarding rules: */
                if (   oldAdapterData.m_attachmentType == KNetworkAttachmentType_NAT
                    || newAdapterData.m_attachmentType == KNetworkAttachmentType_NAT)
                {
                    /* For each rule: */
                    for (int iRule = 0; fSuccess && iRule < m_pCache->child(iSlot).childCount(); ++iRule)
                    {
                        /* Get rule cache: */
                        const UISettingsCachePortForwardingRule &ruleCache = m_pCache->child(iSlot).child(iRule);

                        /* Remove rule marked for 'remove' or 'update': */
                        if (ruleCache.wasRemoved() || ruleCache.wasUpdated())
                        {
                            comEngine.RemoveRedirect(ruleCache.base().name);
                            fSuccess = comEngine.isOk();
                        }
                    }
                    for (int iRule = 0; fSuccess && iRule < m_pCache->child(iSlot).childCount(); ++iRule)
                    {
                        /* Get rule cache: */
                        const UISettingsCachePortForwardingRule &ruleCache = m_pCache->child(iSlot).child(iRule);

                        /* Create rule marked for 'create' or 'update': */
                        if (ruleCache.wasCreated() || ruleCache.wasUpdated())
                        {
                            comEngine.AddRedirect(ruleCache.data().name, ruleCache.data().protocol,
                                                  ruleCache.data().hostIp, ruleCache.data().hostPort.value(),
                                                  ruleCache.data().guestIp, ruleCache.data().guestPort.value());
                            fSuccess = comEngine.isOk();
                        }
                    }

                    /* Show error message if necessary: */
                    if (!fSuccess)
                        notifyOperationProgressError(UIErrorString::formatErrorInfo(comEngine));
                }
            }
        }
    }
    /* Return result: */
    return fSuccess;
}

# include "UIMachineSettingsNetwork.moc"
