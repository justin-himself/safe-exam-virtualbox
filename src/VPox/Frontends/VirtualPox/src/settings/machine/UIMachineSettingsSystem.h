/* $Id: UIMachineSettingsSystem.h $ */
/** @file
 * VPox Qt GUI - UIMachineSettingsSystem class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsSystem_h
#define FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsSystem_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsSystem.gen.h"

/* Forward declarations: */
struct UIDataSettingsMachineSystem;
typedef UISettingsCache<UIDataSettingsMachineSystem> UISettingsCacheMachineSystem;
class CMachine;

/** Machine settings: System page. */
class SHARED_LIBRARY_STUFF UIMachineSettingsSystem : public UISettingsPageMachine,
                                                     public Ui::UIMachineSettingsSystem
{
    Q_OBJECT;

public:

    /** Constructs System settings page. */
    UIMachineSettingsSystem();
    /** Destructs System settings page. */
    ~UIMachineSettingsSystem();

    /** Returns whether the HW Virt Ex is supported. */
    bool isHWVirtExSupported() const;
    /** Returns whether the HW Virt Ex is enabled. */
    bool isHWVirtExEnabled() const;

    /** Returns whether the Nested Paging is supported. */
    bool isNestedPagingSupported() const;
    /** Returns whether the Nested Paging is enabled. */
    bool isNestedPagingEnabled() const;

    /** Returns whether the Nested HW Virt Ex is supported. */
    bool isNestedHWVirtExSupported() const;
    /** Returns whether the Nested HW Virt Ex is enabled. */
    bool isNestedHWVirtExEnabled() const;

    /** Returns whether the HID is enabled. */
    bool isHIDEnabled() const;

    /** Returns the chipset type. */
    KChipsetType chipsetType() const;

    /** Defines whether the USB is enabled. */
    void setUSBEnabled(bool fEnabled);

protected:

    /** Returns whether the page content was changed. */
    virtual bool changed() const /* override */;

    /** Loads data into the cache from corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    virtual void loadToCacheFrom(QVariant &data) /* override */;
    /** Loads data into corresponding widgets from the cache,
      * this task SHOULD be performed in the GUI thread only. */
    virtual void getFromCache() /* override */;

    /** Saves data from corresponding widgets to the cache,
      * this task SHOULD be performed in the GUI thread only. */
    virtual void putToCache() /* override */;
    /** Saves data from the cache to corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    virtual void saveFromCacheTo(QVariant &data) /* overrride */;

    /** Performs validation, updates @a messages list if something is wrong. */
    virtual bool validate(QList<UIValidationMessage> &messages) /* override */;

    /** Defines TAB order for passed @a pWidget. */
    virtual void setOrderAfter(QWidget *pWidget) /* override */;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs final page polishing. */
    virtual void polishPage() /* override */;

private slots:

    /** Handles CPU count slider change. */
    void sltHandleCPUCountSliderChange();
    /** Handles CPU count editor change. */
    void sltHandleCPUCountEditorChange();
    /** Handles CPU execution cap slider change. */
    void sltHandleCPUExecCapSliderChange();
    /** Handles CPU execution cap editor change. */
    void sltHandleCPUExecCapEditorChange();

    /** Handles HW Virt Ex check-box toggling. */
    void sltHandleHwVirtExToggle();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares 'Motherboard' tab. */
    void prepareTabMotherboard();
    /** Prepares 'Processor' tab. */
    void prepareTabProcessor();
    /** Prepares 'Acceleration' tab. */
    void prepareTabAcceleration();
    /** Prepares connections. */
    void prepareConnections();
    /** Cleanups all. */
    void cleanup();

    /** Repopulates Chipset type combo-box. */
    void repopulateComboChipsetType();
    /** Repopulates Pointing HID type combo-box. */
    void repopulateComboPointingHIDType();
    /** Repopulates Paravirtualization Provider type combo-box. */
    void repopulateComboParavirtProviderType();

    /** Retranslates Chipset type combo-box. */
    void retranslateComboChipsetType();
    /** Retranslates Pointing HID type combo-box. */
    void retranslateComboPointingHIDType();
    /** Retranslates Paravirtualization providers combo-box. */
    void retranslateComboParavirtProvider();

    /** Saves existing system data from the cache. */
    bool saveSystemData();
    /** Saves existing 'Motherboard' data from the cache. */
    bool saveMotherboardData();
    /** Saves existing 'Processor' data from the cache. */
    bool saveProcessorData();
    /** Saves existing 'Acceleration' data from the cache. */
    bool saveAccelerationData();

    /** Holds the minimum guest CPU count. */
    uint  m_uMinGuestCPU;
    /** Holds the maximum guest CPU count. */
    uint  m_uMaxGuestCPU;
    /** Holds the minimum guest CPU execution cap. */
    uint  m_uMinGuestCPUExecCap;
    /** Holds the medium guest CPU execution cap. */
    uint  m_uMedGuestCPUExecCap;
    /** Holds the maximum guest CPU execution cap. */
    uint  m_uMaxGuestCPUExecCap;

    /** Holds whether the USB is enabled. */
    bool m_fIsUSBEnabled;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineSystem *m_pCache;
};

#endif /* !FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsSystem_h */
