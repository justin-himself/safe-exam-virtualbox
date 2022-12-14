/* $Id: UIWizardNewVMPageBasic2.h $ */
/** @file
 * VPox Qt GUI - UIWizardNewVMPageBasic2 class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMPageBasic2_h
#define FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMPageBasic2_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Local includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class UIBaseMemorySlider;
class QSpinBox;
class QLabel;
class QIRichTextLabel;

/* 2nd page of the New Virtual Machine wizard (base part): */
class UIWizardNewVMPage2 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardNewVMPage2();

    /* Handlers: */
    void onRamSliderValueChanged();
    void onRamEditorValueChanged();

    /* Widgets: */
    UIBaseMemorySlider *m_pRamSlider;
    QSpinBox *m_pRamEditor;
    QLabel *m_pRamMin;
    QLabel *m_pRamMax;
    QLabel *m_pRamUnits;
};

/* 2nd page of the New Virtual Machine wizard (basic extension): */
class UIWizardNewVMPageBasic2 : public UIWizardPage, public UIWizardNewVMPage2
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIWizardNewVMPageBasic2();

private slots:

    /* Handlers: */
    void sltRamSliderValueChanged();
    void sltRamEditorValueChanged();

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMPageBasic2_h */

