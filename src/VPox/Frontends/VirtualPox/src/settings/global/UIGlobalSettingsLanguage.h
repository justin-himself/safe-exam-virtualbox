/* $Id: UIGlobalSettingsLanguage.h $ */
/** @file
 * VPox Qt GUI - UIGlobalSettingsLanguage class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsLanguage_h
#define FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsLanguage_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIGlobalSettingsLanguage.gen.h"

/* Forward declartions: */
struct UIDataSettingsGlobalLanguage;
typedef UISettingsCache<UIDataSettingsGlobalLanguage> UISettingsCacheGlobalLanguage;

/** Global settings: Language page. */
class SHARED_LIBRARY_STUFF UIGlobalSettingsLanguage : public UISettingsPageGlobal,
                                                      public Ui::UIGlobalSettingsLanguage
{
    Q_OBJECT;

public:

    /** Constructs Language settings page. */
    UIGlobalSettingsLanguage();
    /** Destructs Language settings page. */
    ~UIGlobalSettingsLanguage();

protected:

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

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Handles show @a pEvent. */
    virtual void showEvent(QShowEvent *pEvent) /* override */;
    /** Performs final page polishing. */
    virtual void polishEvent(QShowEvent *pEvent) /* override */;

private slots:

    /** Handles @a pItem painting with passed @a pPainter. */
    void sltHandleItemPainting(QTreeWidgetItem *pItem, QPainter *pPainter);

    /** Handles @a pCurrentItem change. */
    void sltHandleCurrentItemChange(QTreeWidgetItem *pCurrentItem);

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Reloads language list, choosing item with @a strLanguageId as current. */
    void reloadLanguageTree(const QString &strLanguageId);

    /** Saves existing language data from the cache. */
    bool saveLanguageData();

    /** Holds whether the page is polished. */
    bool m_fPolished;

    /** Holds the page data cache instance. */
    UISettingsCacheGlobalLanguage *m_pCache;
};

#endif /* !FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsLanguage_h */
