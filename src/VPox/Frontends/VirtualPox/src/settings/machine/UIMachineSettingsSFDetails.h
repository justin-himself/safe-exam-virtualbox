/* $Id: UIMachineSettingsSFDetails.h $ */
/** @file
 * VPox Qt GUI - UIMachineSettingsSFDetails class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsSFDetails_h
#define FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsSFDetails_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Includes */
#include "UIMachineSettingsSFDetails.gen.h"
#include "QIDialog.h"
#include "QIWithRetranslateUI.h"
#include "UIMachineSettingsSF.h"

/* Shared folders details dialog: */
class SHARED_LIBRARY_STUFF UIMachineSettingsSFDetails : public QIWithRetranslateUI2<QIDialog>,
                                                        public Ui::UIMachineSettingsSFDetails
{
    Q_OBJECT;

public:

    enum SFDialogType
    {
        AddType,
        EditType
    };

    UIMachineSettingsSFDetails(SFDialogType type,
                               bool fEnableSelector, /* for "permanent" checkbox */
                               const QStringList &usedNames,
                               QWidget *pParent = 0);

    void setPath(const QString &strPath);
    QString path() const;

    void setName(const QString &strName);
    QString name() const;

    void setWriteable(bool fWriteable);
    bool isWriteable() const;

    void setAutoMount(bool fAutoMount);
    bool isAutoMounted() const;

    void setAutoMountPoint(const QString &strAutoMountPoint);
    QString autoMountPoint() const;

    void setPermanent(bool fPermanent);
    bool isPermanent() const;

protected:

    void retranslateUi();

private slots:

    void sltSelectPath();
    void sltValidate();

private:

    SFDialogType  m_type;
    bool          m_fUsePermanent;
    QStringList   m_usedNames;
};

#endif /* !FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsSFDetails_h */
