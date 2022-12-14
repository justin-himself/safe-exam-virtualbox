/* $Id: UIActionPoolManager.h $ */
/** @file
 * VPox Qt GUI - UIActionPoolManager class declaration.
 */

/*
 * Copyright (C) 2010-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_globals_UIActionPoolManager_h
#define FEQT_INCLUDED_SRC_globals_UIActionPoolManager_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QList>

/* GUI includes: */
#include "UIActionPool.h"
#include "UILibraryDefs.h"


/** Runtime action-pool index enum.
  * Naming convention is following:
  * 1. Every menu index prepended with 'M',
  * 2. Every simple-action index prepended with 'S',
  * 3. Every toggle-action index presended with 'T',
  * 4. Every polymorphic-action index presended with 'P',
  * 5. Every sub-index contains full parent-index name. */
enum UIActionIndexST
{
    /* 'File' menu actions: */
    UIActionIndexST_M_File = UIActionIndex_Max + 1,
    UIActionIndexST_M_File_S_ShowVirtualMediumManager,
    UIActionIndexST_M_File_S_ShowHostNetworkManager,
    UIActionIndexST_M_File_S_ShowCloudProfileManager,
    UIActionIndexST_M_File_S_ImportAppliance,
    UIActionIndexST_M_File_S_ExportAppliance,
    UIActionIndexST_M_File_S_NewCloudVM,
#ifdef VPOX_GUI_WITH_EXTRADATA_MANAGER_UI
    UIActionIndexST_M_File_S_ShowExtraDataManager,
#endif
    UIActionIndexST_M_File_S_Close,

    /* 'Welcome' menu actions: */
    UIActionIndexST_M_Welcome,
    UIActionIndexST_M_Welcome_S_New,
    UIActionIndexST_M_Welcome_S_Add,

    /* 'Group' menu actions: */
    UIActionIndexST_M_Group,
    UIActionIndexST_M_Group_S_New,
    UIActionIndexST_M_Group_S_Add,
    UIActionIndexST_M_Group_S_Rename,
    UIActionIndexST_M_Group_S_Remove,
    UIActionIndexST_M_Group_M_StartOrShow,
    UIActionIndexST_M_Group_M_StartOrShow_S_StartNormal,
    UIActionIndexST_M_Group_M_StartOrShow_S_StartHeadless,
    UIActionIndexST_M_Group_M_StartOrShow_S_StartDetachable,
    UIActionIndexST_M_Group_T_Pause,
    UIActionIndexST_M_Group_S_Reset,
    UIActionIndexST_M_Group_M_Close,
    UIActionIndexST_M_Group_M_Close_S_Detach,
    UIActionIndexST_M_Group_M_Close_S_SaveState,
    UIActionIndexST_M_Group_M_Close_S_Shutdown,
    UIActionIndexST_M_Group_M_Close_S_PowerOff,
    UIActionIndexST_M_Group_M_Tools,
    UIActionIndexST_M_Group_M_Tools_T_Details,
    UIActionIndexST_M_Group_M_Tools_T_Snapshots,
    UIActionIndexST_M_Group_M_Tools_T_Logs,
    UIActionIndexST_M_Group_S_Discard,
    UIActionIndexST_M_Group_S_ShowLogDialog,
    UIActionIndexST_M_Group_S_Refresh,
    UIActionIndexST_M_Group_S_ShowInFileManager,
    UIActionIndexST_M_Group_S_CreateShortcut,
    UIActionIndexST_M_Group_S_Sort,

    /* 'Machine' menu actions: */
    UIActionIndexST_M_Machine,
    UIActionIndexST_M_Machine_S_New,
    UIActionIndexST_M_Machine_S_Add,
    UIActionIndexST_M_Machine_S_Settings,
    UIActionIndexST_M_Machine_S_Clone,
    UIActionIndexST_M_Machine_S_Move,
    UIActionIndexST_M_Machine_S_ExportToOCI,
    UIActionIndexST_M_Machine_S_Remove,
    UIActionIndexST_M_Machine_S_AddGroup,
    UIActionIndexST_M_Machine_M_StartOrShow,
    UIActionIndexST_M_Machine_M_StartOrShow_S_StartNormal,
    UIActionIndexST_M_Machine_M_StartOrShow_S_StartHeadless,
    UIActionIndexST_M_Machine_M_StartOrShow_S_StartDetachable,
    UIActionIndexST_M_Machine_T_Pause,
    UIActionIndexST_M_Machine_S_Reset,
    UIActionIndexST_M_Machine_M_Close,
    UIActionIndexST_M_Machine_M_Close_S_Detach,
    UIActionIndexST_M_Machine_M_Close_S_SaveState,
    UIActionIndexST_M_Machine_M_Close_S_Shutdown,
    UIActionIndexST_M_Machine_M_Close_S_PowerOff,
    UIActionIndexST_M_Machine_M_Tools,
    UIActionIndexST_M_Machine_M_Tools_T_Details,
    UIActionIndexST_M_Machine_M_Tools_T_Snapshots,
    UIActionIndexST_M_Machine_M_Tools_T_Logs,
    UIActionIndexST_M_Machine_S_Discard,
    UIActionIndexST_M_Machine_S_ShowLogDialog,
    UIActionIndexST_M_Machine_S_Refresh,
    UIActionIndexST_M_Machine_S_ShowInFileManager,
    UIActionIndexST_M_Machine_S_CreateShortcut,
    UIActionIndexST_M_Machine_S_SortParent,
    UIActionIndexST_M_Machine_S_Search,

    /* Global Tools actions: */
    UIActionIndexST_M_Tools_M_Global,
    UIActionIndexST_M_Tools_M_Global_S_VirtualMediaManager,
    UIActionIndexST_M_Tools_M_Global_S_HostNetworkManager,
    UIActionIndexST_M_Tools_M_Global_S_CloudProfileManager,

    /* Snapshot Pane actions: */
    UIActionIndexST_M_Snapshot,
    UIActionIndexST_M_Snapshot_S_Take,
    UIActionIndexST_M_Snapshot_S_Delete,
    UIActionIndexST_M_Snapshot_S_Restore,
    UIActionIndexST_M_Snapshot_T_Properties,
    UIActionIndexST_M_Snapshot_S_Clone,

    /* Virtual Media Manager actions: */
    UIActionIndexST_M_MediumWindow,
    UIActionIndexST_M_Medium,
    UIActionIndexST_M_Medium_S_Add,
    UIActionIndexST_M_Medium_S_Create,
    UIActionIndexST_M_Medium_S_Copy,
    UIActionIndexST_M_Medium_S_Move,
    UIActionIndexST_M_Medium_S_Remove,
    UIActionIndexST_M_Medium_S_Release,
    UIActionIndexST_M_Medium_T_Details,
    UIActionIndexST_M_Medium_T_Search,
    UIActionIndexST_M_Medium_S_Refresh,

    /* Host Network Manager actions: */
    UIActionIndexST_M_NetworkWindow,
    UIActionIndexST_M_Network,
    UIActionIndexST_M_Network_S_Create,
    UIActionIndexST_M_Network_S_Remove,
    UIActionIndexST_M_Network_T_Details,
    UIActionIndexST_M_Network_S_Refresh,

    /* Cloud Profile Manager actions: */
    UIActionIndexST_M_CloudWindow,
    UIActionIndexST_M_Cloud,
    UIActionIndexST_M_Cloud_S_Add,
    UIActionIndexST_M_Cloud_S_Import,
    UIActionIndexST_M_Cloud_S_Remove,
    UIActionIndexST_M_Cloud_T_Details,
    UIActionIndexST_M_Cloud_S_TryPage,
    UIActionIndexST_M_Cloud_S_Help,

    /* Maximum index: */
    UIActionIndexST_Max
};


/** UIActionPool extension
  * representing action-pool singleton for Manager UI. */
class SHARED_LIBRARY_STUFF UIActionPoolManager : public UIActionPool
{
    Q_OBJECT;

protected:

    /** Constructs action-pool.
      * @param  fTemporary  Brings whether this action-pool is temporary,
      *                     used to (re-)initialize shortcuts-pool. */
    UIActionPoolManager(bool fTemporary = false);

    /** Prepares pool. */
    virtual void preparePool() /* override */;
    /** Prepares connections. */
    virtual void prepareConnections() /* override */;

    /** Updates menu. */
    virtual void updateMenu(int iIndex) /* override */;
    /** Updates menus. */
    virtual void updateMenus() /* override */;

    /** Updates 'File' menu. */
    void updateMenuFile();
    /** Updates 'Welcome' menu. */
    void updateMenuWelcome();
    /** Updates 'Group' menu. */
    void updateMenuGroup();
    /** Updates 'Machine' menu. */
    void updateMenuMachine();
    /** Updates 'Group' / 'Start or Show' menu. */
    void updateMenuGroupStartOrShow();
    /** Updates 'Machine' / 'Start or Show' menu. */
    void updateMenuMachineStartOrShow();
    /** Updates 'Group' / 'Close' menu. */
    void updateMenuGroupClose();
    /** Updates 'Machine' / 'Close' menu. */
    void updateMenuMachineClose();
    /** Updates 'Group' / 'Tools' menu. */
    void updateMenuGroupTools();
    /** Updates 'Machine' / 'Tools' menu. */
    void updateMenuMachineTools();

    /** Updates 'Medium' window menu. */
    void updateMenuMediumWindow();
    /** Updates 'Medium' menu. */
    void updateMenuMedium();
    /** Updates 'Medium' @a pMenu. */
    void updateMenuMediumWrapper(UIMenu *pMenu);

    /** Updates 'Network' window menu. */
    void updateMenuNetworkWindow();
    /** Updates 'Network' menu. */
    void updateMenuNetwork();
    /** Updates 'Network' @a pMenu. */
    void updateMenuNetworkWrapper(UIMenu *pMenu);

    /** Updates 'Cloud' window menu. */
    void updateMenuCloudWindow();
    /** Updates 'Cloud' menu. */
    void updateMenuCloud();
    /** Updates 'Cloud' @a pMenu. */
    void updateMenuCloudWrapper(UIMenu *pMenu);

    /** Updates 'Snapshot' menu. */
    void updateMenuSnapshot();

    /** Updates shortcuts. */
    virtual void updateShortcuts() /* override */;

    /** Defines whether shortcuts of menu actions with specified @a iIndex should be visible. */
    virtual void setShortcutsVisible(int iIndex, bool fVisible) /* override */;

    /** Returns extra-data ID to save keyboard shortcuts under. */
    virtual QString shortcutsExtraDataID() const /* override */;

private:

    /** Enables factory in base-class. */
    friend class UIActionPool;
};


#endif /* !FEQT_INCLUDED_SRC_globals_UIActionPoolManager_h */
