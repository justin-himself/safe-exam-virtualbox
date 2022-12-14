/* $Id: UIWizardNewVMPageExpert.h $ */
/** @file
 * VPox Qt GUI - UIWizardNewVMPageExpert class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMPageExpert_h
#define FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMPageExpert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Local includes: */
#include "UIWizardNewVMPageBasic1.h"
#include "UIWizardNewVMPageBasic2.h"
#include "UIWizardNewVMPageBasic3.h"

/* Forward declarations: */
class QGroupBox;

/* Expert page of the New Virtual Machine wizard: */
class UIWizardNewVMPageExpert : public UIWizardPage,
                                public UIWizardNewVMPage1,
                                public UIWizardNewVMPage2,
                                public UIWizardNewVMPage3
{
    Q_OBJECT;
    Q_PROPERTY(QString machineFolder READ machineFolder WRITE setMachineFolder);
    Q_PROPERTY(QString machineBaseName READ machineBaseName WRITE setMachineBaseName);
    Q_PROPERTY(QString machineFilePath READ machineFilePath WRITE setMachineFilePath);
    Q_PROPERTY(CMedium virtualDisk READ virtualDisk WRITE setVirtualDisk);
    Q_PROPERTY(QUuid virtualDiskId READ virtualDiskId WRITE setVirtualDiskId);
    Q_PROPERTY(QString virtualDiskLocation READ virtualDiskLocation WRITE setVirtualDiskLocation);

public:

    /* Constructor: */
    UIWizardNewVMPageExpert(const QString &strGroup);

protected:

    /* Wrapper to access 'wizard' from base part: */
    UIWizard *wizardImp() const { return wizard(); }
    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }
    /* Wrapper to access 'wizard-field' from base part: */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:

    /* Handlers: */
    void sltNameChanged(const QString &strNewText);
    void sltPathChanged(const QString &strNewPath);
    void sltOsTypeChanged();
    void sltRamSliderValueChanged();
    void sltRamEditorValueChanged();
    void sltVirtualDiskSourceChanged();
    void sltGetWithFileOpenDialog();

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();
    void cleanupPage();

    /* Validation stuff: */
    bool isComplete() const;
    bool validatePage();

    /* Widgets: */
    QGroupBox *m_pNameAndSystemCnt;
    QGroupBox *m_pMemoryCnt;
    QGroupBox *m_pDiskCnt;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMPageExpert_h */
