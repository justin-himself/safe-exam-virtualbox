/* $Id: UIVirtualPoxManagerWidget.h $ */
/** @file
 * VPox Qt GUI - UIVirtualPoxManagerWidget class declaration.
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

#ifndef FEQT_INCLUDED_SRC_manager_UIVirtualPoxManagerWidget_h
#define FEQT_INCLUDED_SRC_manager_UIVirtualPoxManagerWidget_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UISlidingAnimation.h"
#include "UIToolPaneGlobal.h"
#include "UIToolPaneMachine.h"

/* Forward declarations: */
class QStackedWidget;
class QISplitter;
class UIActionPool;
class UIChooser;
class UITabBar;
class UIToolBar;
class UITools;
class UIVirtualPoxManager;
class UIVirtualMachineItem;

/** QWidget extension used as VirtualPox Manager Widget instance. */
class UIVirtualPoxManagerWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

    /** Possible selection types. */
    enum SelectionType
    {
        SelectionType_Invalid,
        SelectionType_SingleGroupItem,
        SelectionType_FirstIsGlobalItem,
        SelectionType_FirstIsMachineItem
    };

signals:

    /** Notifies about Chooser-pane index change. */
    void sigChooserPaneIndexChange();
    /** Notifies about Chooser-pane group saving change. */
    void sigGroupSavingStateChanged();

    /** Notifies aboud Details-pane link clicked. */
    void sigMachineSettingsLinkClicked(const QString &strCategory, const QString &strControl, const QUuid &uId);

    /** Notifies about Tool type change. */
    void sigToolTypeChange();

    /** Notifies listeners about Cloud Profile Manager change. */
    void sigCloudProfileManagerChange();

    /** Notifies listeners about current Snapshots pane item change. */
    void sigCurrentSnapshotItemChange();

    /** Notifies about state change for cloud machine with certain @a strMachineId. */
    void sigCloudMachineStateChange(const QString &strMachineId);

public:

    /** Constructs VirtualPox Manager widget. */
    UIVirtualPoxManagerWidget(UIVirtualPoxManager *pParent);
    /** Destructs VirtualPox Manager widget. */
    virtual ~UIVirtualPoxManagerWidget() /* override */;

    /** @name Common stuff.
      * @{ */
        /** Returns the action-pool instance. */
        UIActionPool *actionPool() const { return m_pActionPool; }

        /** Returns whether group current-item is selected. */
        bool isGroupItemSelected() const;
        /** Returns whether global current-item is selected. */
        bool isGlobalItemSelected() const;
        /** Returns whether machine current-item is selected. */
        bool isMachineItemSelected() const;
        /** Returns current-item. */
        UIVirtualMachineItem *currentItem() const;
        /** Returns a list of current-items. */
        QList<UIVirtualMachineItem*> currentItems() const;

        /** Returns whether group saving is in progress. */
        bool isGroupSavingInProgress() const;
        /** Returns whether all items of one group is selected. */
        bool isAllItemsOfOneGroupSelected() const;
        /** Returns whether single group is selected. */
        bool isSingleGroupSelected() const;

        /** Defines tools @a enmType. */
        void setToolsType(UIToolType enmType);
        /** Returns tools type. */
        UIToolType toolsType() const;

        /** Returns a type of curent Global tool. */
        UIToolType currentGlobalTool() const;
        /** Returns a type of curent Machine tool. */
        UIToolType currentMachineTool() const;
        /** Returns whether Global tool of passed @a enmType is opened. */
        bool isGlobalToolOpened(UIToolType enmType) const;
        /** Returns whether Machine tool of passed @a enmType is opened. */
        bool isMachineToolOpened(UIToolType enmType) const;
        /** Switches to Global tool of passed @a enmType. */
        void switchToGlobalTool(UIToolType enmType);
        /** Switches to Machine tool of passed @a enmType. */
        void switchToMachineTool(UIToolType enmType);
        /** Closes Global tool of passed @a enmType. */
        void closeGlobalTool(UIToolType enmType);
        /** Closes Machine tool of passed @a enmType. */
        void closeMachineTool(UIToolType enmType);
    /** @} */

    /** @name Snapshot pane stuff.
      * @{ */
        /** Returns whether current-state item of Snapshot pane is selected. */
        bool isCurrentStateItemSelected() const;
    /** @} */

public slots:

    /** @name Common stuff.
      * @{ */
        /** Handles context-menu request for passed @a position. */
        void sltHandleContextMenuRequest(const QPoint &position);
    /** @} */

protected:

    /** @name Event handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;
    /** @} */

private slots:

    /** @name Common stuff.
      * @{ */
        /** Handles signal about Chooser-pane index change. */
        void sltHandleChooserPaneIndexChange();

        /** Handles signal about Chooser-pane selection invalidated. */
        void sltHandleChooserPaneSelectionInvalidated() { recacheCurrentItemInformation(true /* fDontRaiseErrorPane */); }

        /** Handles sliding animation complete signal.
          * @param  enmDirection  Brings which direction was animation finished for. */
        void sltHandleSlidingAnimationComplete(SlidingDirection enmDirection);

        /** Handles state change for cloud machine with specified @a strMachineId. */
        void sltHandleCloudMachineStateChange(const QString &strId);
    /** @} */

    /** @name Tools stuff.
      * @{ */
        /** Handles tool menu request. */
        void sltHandleToolMenuRequested(UIToolClass enmClass, const QPoint &position);

        /** Handles signal abour Tools-pane index change. */
        void sltHandleToolsPaneIndexChange();
    /** @} */

private:

    /** @name Prepare/Cleanup cascade.
      * @{ */
        /** Prepares window. */
        void prepare();
        /** Prepares widgets. */
        void prepareWidgets();
        /** Prepares connections. */
        void prepareConnections();
        /** Loads settings. */
        void loadSettings();

        /** Update toolbar. */
        void updateToolbar();

        /** Saves settings. */
        void saveSettings();
        /** Cleanups window. */
        void cleanup();
    /** @} */

    /** @name Common stuff.
      * @{ */
        /** Recaches current item information.
          * @param  fDontRaiseErrorPane  Brings whether we should not raise error-pane. */
        void recacheCurrentItemInformation(bool fDontRaiseErrorPane = false);
    /** @} */

    /** Holds the action-pool instance. */
    UIActionPool *m_pActionPool;

    /** Holds the central splitter instance. */
    QISplitter *m_pSplitter;

    /** Holds the main toolbar instance. */
    UIToolBar *m_pToolBar;

    /** Holds the Chooser-pane instance. */
    UIChooser          *m_pPaneChooser;
    /** Holds the stacked-widget. */
    QStackedWidget     *m_pStackedWidget;
    /** Holds the Global Tools-pane instance. */
    UIToolPaneGlobal   *m_pPaneToolsGlobal;
    /** Holds the Machine Tools-pane instance. */
    UIToolPaneMachine  *m_pPaneToolsMachine;
    /** Holds the sliding-animation widget instance. */
    UISlidingAnimation *m_pSlidingAnimation;
    /** Holds the Tools-pane instance. */
    UITools            *m_pPaneTools;

    /** Holds the last selection type. */
    SelectionType  m_enmSelectionType;
    /** Holds whether the last selected item was accessible. */
    bool           m_fSelectedMachineItemAccessible;
};

#endif /* !FEQT_INCLUDED_SRC_manager_UIVirtualPoxManagerWidget_h */
