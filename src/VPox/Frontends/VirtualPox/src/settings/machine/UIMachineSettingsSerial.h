/* $Id: UIMachineSettingsSerial.h $ */
/** @file
 * VPox Qt GUI - UIMachineSettingsSerial class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsSerial_h
#define FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsSerial_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsSerial.gen.h"

/* Forward declarations: */
class QITabWidget;
class UIMachineSettingsSerialPage;
struct UIDataSettingsMachineSerial;
struct UIDataSettingsMachineSerialPort;
typedef UISettingsCache<UIDataSettingsMachineSerialPort> UISettingsCacheMachineSerialPort;
typedef UISettingsCachePool<UIDataSettingsMachineSerial, UISettingsCacheMachineSerialPort> UISettingsCacheMachineSerial;

/** Machine settings: Serial page. */
class SHARED_LIBRARY_STUFF UIMachineSettingsSerialPage : public UISettingsPageMachine
{
    Q_OBJECT;

public:

    /** Constructs Serial settings page. */
    UIMachineSettingsSerialPage();
    /** Destructs Serial settings page. */
    ~UIMachineSettingsSerialPage();

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

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs final page polishing. */
    virtual void polishPage() /* override */;

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Saves existing serial data from the cache. */
    bool saveSerialData();
    /** Saves existing port data from the cache. */
    bool savePortData(int iSlot);

    /** Holds the tab-widget instance. */
    QITabWidget *m_pTabWidget;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineSerial *m_pCache;
};

#endif /* !FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsSerial_h */
