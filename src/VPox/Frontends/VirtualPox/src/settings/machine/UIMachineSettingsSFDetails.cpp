/* $Id: UIMachineSettingsSFDetails.cpp $ */
/** @file
 * VPox Qt GUI - UIMachineSettingsSFDetails class implementation.
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

/* Qt includes */
#include <QDir>
#include <QPushButton>

/* Other includes */
#include "UIMachineSettingsSFDetails.h"
#include "UICommon.h"


UIMachineSettingsSFDetails::UIMachineSettingsSFDetails(SFDialogType type,
                                                       bool fEnableSelector, /* for "permanent" checkbox */
                                                       const QStringList &usedNames,
                                                       QWidget *pParent /* = 0 */)
   : QIWithRetranslateUI2<QIDialog>(pParent)
   , m_type(type)
   , m_fUsePermanent(fEnableSelector)
   , m_usedNames(usedNames)
{
    /* Apply UI decorations: */
    Ui::UIMachineSettingsSFDetails::setupUi(this);

    /* Setup widgets: */
    mPsPath->setResetEnabled(false);
    mPsPath->setHomeDir(QDir::homePath());
    mCbPermanent->setHidden(!fEnableSelector);

    /* Setup connections: */
    connect(mPsPath, static_cast<void(UIFilePathSelector::*)(int)>(&UIFilePathSelector::currentIndexChanged),
            this, &UIMachineSettingsSFDetails::sltSelectPath);
    connect(mPsPath, &UIFilePathSelector::pathChanged, this, &UIMachineSettingsSFDetails::sltSelectPath);
    connect(mLeName, &QLineEdit::textChanged, this, &UIMachineSettingsSFDetails::sltValidate);
    if (fEnableSelector)
        connect(mCbPermanent, &QCheckBox::toggled, this, &UIMachineSettingsSFDetails::sltValidate);

     /* Applying language settings: */
    retranslateUi();

    /* Validate the initial field values: */
    sltValidate();

    /* Adjust dialog size: */
    adjustSize();

#ifdef VPOX_WS_MAC
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(minimumSize());
#endif /* VPOX_WS_MAC */
}

void UIMachineSettingsSFDetails::setPath(const QString &strPath)
{
    mPsPath->setPath(strPath);
}

QString UIMachineSettingsSFDetails::path() const
{
    return mPsPath->path();
}

void UIMachineSettingsSFDetails::setName(const QString &strName)
{
    mLeName->setText(strName);
}

QString UIMachineSettingsSFDetails::name() const
{
    return mLeName->text();
}

void UIMachineSettingsSFDetails::setWriteable(bool fWritable)
{
    mCbReadonly->setChecked(!fWritable);
}

bool UIMachineSettingsSFDetails::isWriteable() const
{
    return !mCbReadonly->isChecked();
}

void UIMachineSettingsSFDetails::setAutoMount(bool fAutoMount)
{
    mCbAutoMount->setChecked(fAutoMount);
}

bool UIMachineSettingsSFDetails::isAutoMounted() const
{
    return mCbAutoMount->isChecked();
}

void UIMachineSettingsSFDetails::setAutoMountPoint(const QString &strAutoMountPoint)
{
    mLeAutoMountPoint->setText(strAutoMountPoint);
}

QString UIMachineSettingsSFDetails::autoMountPoint() const
{
    return mLeAutoMountPoint->text();
}

void UIMachineSettingsSFDetails::setPermanent(bool fPermanent)
{
    mCbPermanent->setChecked(fPermanent);
}

bool UIMachineSettingsSFDetails::isPermanent() const
{
    return m_fUsePermanent ? mCbPermanent->isChecked() : true;
}

void UIMachineSettingsSFDetails::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIMachineSettingsSFDetails::retranslateUi(this);

    switch (m_type)
    {
        case AddType:
            setWindowTitle(tr("Add Share"));
            break;
        case EditType:
            setWindowTitle(tr("Edit Share"));
            break;
        default:
            AssertMsgFailed(("Incorrect shared-folders dialog type: %d\n", m_type));
    }
}

void UIMachineSettingsSFDetails::sltValidate()
{
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!mPsPath->path().isEmpty() &&
                                                         QDir(mPsPath->path()).exists() &&
                                                         !mLeName->text().trimmed().isEmpty() &&
                                                         !mLeName->text().contains(" ") &&
                                                         !m_usedNames.contains(mLeName->text()));
}

void UIMachineSettingsSFDetails::sltSelectPath()
{
    if (!mPsPath->isPathSelected())
        return;

    QString strFolderName(mPsPath->path());
#if defined (VPOX_WS_WIN) || defined (Q_OS_OS2)
    if (strFolderName[0].isLetter() && strFolderName[1] == ':' && strFolderName[2] == 0)
    {
        /* UIFilePathSelector returns root path as 'X:', which is invalid path.
         * Append the trailing backslash to get a valid root path 'X:\': */
        strFolderName += "\\";
        mPsPath->setPath(strFolderName);
    }
#endif /* VPOX_WS_WIN || Q_OS_OS2 */
    QDir folder(strFolderName);
    if (!folder.isRoot())
    {
        /* Processing non-root folder */
        mLeName->setText(folder.dirName().replace(' ', '_'));
    }
    else
    {
        /* Processing root folder: */
#if defined (VPOX_WS_WIN) || defined (Q_OS_OS2)
        mLeName->setText(strFolderName.toUpper()[0] + "_DRIVE");
#elif defined (VPOX_WS_X11)
        mLeName->setText("ROOT");
#endif
    }
    /* Validate the field values: */
    sltValidate();
}
