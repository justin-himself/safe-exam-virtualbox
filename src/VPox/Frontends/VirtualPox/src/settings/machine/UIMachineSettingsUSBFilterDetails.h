/* $Id: UIMachineSettingsUSBFilterDetails.h $ */
/** @file
 * VPox Qt GUI - UIMachineSettingsUSBFilterDetails class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsUSBFilterDetails_h
#define FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsUSBFilterDetails_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "UIMachineSettingsUSBFilterDetails.gen.h"
#include "QIWithRetranslateUI.h"
#include "QIDialog.h"
#include "UIMachineSettingsUSB.h"

class SHARED_LIBRARY_STUFF UIMachineSettingsUSBFilterDetails : public QIWithRetranslateUI2<QIDialog>,
                                                               public Ui::UIMachineSettingsUSBFilterDetails
{
    Q_OBJECT;

public:

    UIMachineSettingsUSBFilterDetails(QWidget *pParent = 0);

private:

    void retranslateUi();
};

#endif /* !FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsUSBFilterDetails_h */
