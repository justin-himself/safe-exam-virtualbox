/* $Id: UIChooserView.cpp $ */
/** @file
 * VPox Qt GUI - UIChooserView class implementation.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#include <QAccessibleWidget>
#include <QScrollBar>

/* GUI includes: */
#include "UIChooser.h"
#include "UIChooserItem.h"
#include "UIChooserModel.h"
#include "UIChooserSearchWidget.h"
#include "UIChooserView.h"

/* Other VPox includes: */
#include <iprt/assert.h>


/** QAccessibleWidget extension used as an accessibility interface for Chooser-view. */
class UIAccessibilityInterfaceForUIChooserView : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating Chooser-view accessibility interface: */
        if (pObject && strClassname == QLatin1String("UIChooserView"))
            return new UIAccessibilityInterfaceForUIChooserView(qobject_cast<QWidget*>(pObject));

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    UIAccessibilityInterfaceForUIChooserView(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::List)
    {}

    /** Returns the number of children. */
    virtual int childCount() const /* override */
    {
        /* Make sure view still alive: */
        AssertPtrReturn(view(), 0);

        /* Return the number of children: */
        return view()->chooser()->model()->root()->items().size();
    }

    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int iIndex) const /* override */
    {
        /* Make sure view still alive: */
        AssertPtrReturn(view(), 0);
        /* Make sure index is valid: */
        AssertReturn(iIndex >= 0 && iIndex < childCount(), 0);

        /* Return the child with the passed iIndex: */
        return QAccessible::queryAccessibleInterface(view()->chooser()->model()->root()->items().at(iIndex));
    }

    /** Returns a text for the passed @a enmTextRole. */
    virtual QString text(QAccessible::Text enmTextRole) const /* override */
    {
        /* Make sure view still alive: */
        AssertPtrReturn(view(), QString());

        /* Return view tool-tip: */
        Q_UNUSED(enmTextRole);
        return view()->toolTip();
    }

private:

    /** Returns corresponding Chooser-view. */
    UIChooserView *view() const { return qobject_cast<UIChooserView*>(widget()); }
};


UIChooserView::UIChooserView(UIChooser *pParent)
    : QIWithRetranslateUI<QIGraphicsView>(pParent)
    , m_pChooser(pParent)
    , m_pSearchWidget(0)
    , m_iMinimumWidthHint(0)
{
    /* Prepare: */
    prepare();
}

bool UIChooserView::isSearchWidgetVisible() const
{
    if (!m_pSearchWidget)
        return false;

    /* Return widget visibility state: */
    return m_pSearchWidget->isVisible();
}

void UIChooserView::setSearchWidgetVisible(bool fVisible)
{
    if (!m_pSearchWidget)
        return;

    /* Make sure keyboard focus is managed correctly: */
    if (fVisible)
        m_pSearchWidget->setFocus();
    else
        setFocus();

    /* Make sure visibility state is really changed: */
    if (m_pSearchWidget->isVisible() == fVisible)
        return;

    /* Set widget visibility state: */
    m_pSearchWidget->setVisible(fVisible);

    /* Update geometry if widget is visible: */
    if (m_pSearchWidget->isVisible())
        updateSearchWidgetGeometry();

    /* Reset search each time widget visibility changed: */
    UIChooserModel *pModel = m_pChooser->model();
    if (pModel)
        pModel->resetSearch();
}

void UIChooserView::setSearchResultsCount(int iTotalMacthCount, int iCurrentlyScrolledItemIndex)
{
    if (!m_pSearchWidget)
        return;

    m_pSearchWidget->setMatchCount(iTotalMacthCount);
    m_pSearchWidget->setScroolToIndex(iCurrentlyScrolledItemIndex);
}

void UIChooserView::appendToSearchString(const QString &strSearchText)
{
    if (!m_pSearchWidget)
        return;

    m_pSearchWidget->appendToSearchString(strSearchText);
}

void UIChooserView::sltMinimumWidthHintChanged(int iHint)
{
    /* Is there something changed? */
    if (m_iMinimumWidthHint == iHint)
        return;

    /* Remember new value: */
    m_iMinimumWidthHint = iHint;

    /* Set minimum view width according passed width-hint: */
    setMinimumWidth(2 * frameWidth() + m_iMinimumWidthHint + verticalScrollBar()->sizeHint().width());

    /* Update scene-rect: */
    updateSceneRect();
}

void UIChooserView::sltRedoSearch(const QString &strSearchTerm, int iItemSearchFlags)
{
    if (!m_pChooser)
        return;
    UIChooserModel *pModel =  m_pChooser->model();
    if (!pModel)
        return;

    pModel->performSearch(strSearchTerm, iItemSearchFlags);
}

void UIChooserView::sltHandleScrollToSearchResult(bool fIsNext)
{
    if (!m_pChooser)
        return;
    UIChooserModel *pModel =  m_pChooser->model();
    if (!pModel)
        return;

    pModel->selectSearchResult(fIsNext);
}

void UIChooserView::sltHandleSearchWidgetVisibilityToggle(bool fIsVisible)
{
    setSearchWidgetVisible(fIsVisible);
}

void UIChooserView::retranslateUi()
{
    /* Translate this: */
#if 0 /* we will leave that for accessibility needs. */
    setToolTip(tr("Contains a tree of Virtual Machines and their groups"));
#endif  /* to be integrated to accessibility interface. */
}

void UIChooserView::prepare()
{
    /* Install Chooser-view accessibility interface factory: */
    QAccessible::installFactory(UIAccessibilityInterfaceForUIChooserView::pFactory);

    /* Prepare palette: */
    preparePalette();

    /* Setup frame: */
    setFrameShape(QFrame::NoFrame);
    setFrameShadow(QFrame::Plain);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    /* Setup scroll-bars policy: */
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /* Create the search widget (hidden): */
    m_pSearchWidget = new UIChooserSearchWidget(this);
    m_pSearchWidget->hide();
    connect(m_pSearchWidget, &UIChooserSearchWidget::sigRedoSearch,
            this, &UIChooserView::sltRedoSearch);
    connect(m_pSearchWidget, &UIChooserSearchWidget::sigScrollToMatch,
            this, &UIChooserView::sltHandleScrollToSearchResult);
    connect(m_pSearchWidget, &UIChooserSearchWidget::sigToggleVisibility,
            this, &UIChooserView::sltHandleSearchWidgetVisibilityToggle);

    /* Update scene-rect: */
    updateSceneRect();
    /* Update the location and size of the search widget: */
    updateSearchWidgetGeometry();

    /* Apply language settings: */
    retranslateUi();
}

void UIChooserView::preparePalette()
{
    /* Setup palette: */
    QPalette pal = qApp->palette();
    const QColor bodyColor = pal.color(QPalette::Active, QPalette::Midlight).darker(110);
    pal.setColor(QPalette::Base, bodyColor);
    setPalette(pal);
}

void UIChooserView::resizeEvent(QResizeEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QIGraphicsView>::resizeEvent(pEvent);
    /* Notify listeners: */
    emit sigResized();

    /* Update scene-rect: */
    updateSceneRect();
    /* Update the location and size of the search widget: */
    updateSearchWidgetGeometry();
}

void UIChooserView::updateSceneRect()
{
    setSceneRect(0, 0, m_iMinimumWidthHint, height());
}

void UIChooserView::updateSearchWidgetGeometry()
{
    if (!m_pSearchWidget || !m_pSearchWidget->isVisible())
        return;

    const int iHeight = m_pSearchWidget->height();
    m_pSearchWidget->setGeometry(QRect(0, height() - iHeight, width(), iHeight));
}
