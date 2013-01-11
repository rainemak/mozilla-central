/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QCoreApplication>
#include <QGraphicsLinearLayout>
#include <QGraphicsScene>
#include "mainview.h"
#include <stdlib.h>
#include "embedcontext.h"
#include "viewtab.h"
#include "navbutton.h"
#include "textitem.h"
#include "nsIWebBrowserChrome.h"

static bool scrollMode = getenv("SCROLL_MODE") != 0;
static bool noLayout = getenv("NO_LAYOUT") != 0;
static bool showVkb = getenv("SHOW_VKB") != 0;
static bool noLoadUrls = getenv("NO_LOAD_URLS") != 0;
static bool tiledView = getenv("TILED_VIEW") != 0;
static bool showBars = getenv("HIDE_BARS") == 0;
static int timeoutOpen = getenv("TIMEOUT_OPEN") ? atoi(getenv("TIMEOUT_OPEN")) : 0;
static int timeoutStart = getenv("TIMEOUT_START") ? atoi(getenv("TIMEOUT_START")) : timeoutOpen + 100;

MainView::MainView(EmbedContext* aContext)
    : QGraphicsWidget()
    , mContext(aContext)
    , lMainVertLayout(0)
    , lHorNavButtonBar(0)
    , lTabsViewContainer(0)
    , lTabsTitleBar(0)
    , lView(0)
    , lVkbView(0)
    , lViewVKBCont(0)
    , mCurrentTab(0)
{
    QStringList args = qApp->arguments();
    argIDX = args.indexOf(QString("-url"));
    if (argIDX > 0 && argIDX + 1 < args.size()) {
        argIDX++;
        for (int i = argIDX; i < args.size(); i++) {
            urls.append(args.at(i));
        }
        argIDX = 0;
    }
    if (aContext)
        SetMozContext(aContext);
}

void MainView::closeEvent(QCloseEvent* aEvent)
{
    Q_EMIT onCloseEvent();
}

void MainView::SetMozContext(EmbedContext* aContext)
{
    mContext = aContext;
    connect(mContext, SIGNAL(contextFinalized()), this, SLOT(onContextFinalized()));
}

QGraphicsWidget*
MainView::onRequestNewWindow(QGraphicsWidget* aParent, int flags)
{
    ViewTab* tab = new ViewTab(mContext, boundingRect().toRect().size(), flags, aParent);
    tabsList.append(tab);
    if (lTabsTitleBar) {
        connect(tab, SIGNAL(tabButtonClicked(ViewTab*)), this, SLOT(onTabButtonClicked(ViewTab*)));
        connect(tab, SIGNAL(tabLocationChanged(ViewTab*)), this, SLOT(onTabLocationChanged(ViewTab*)));
        lTabsTitleBar->addItem(tab->button());
        SwitchToTab(tab);
    }
    return tab;
}

void MainView::onTabButtonClicked(ViewTab* tab)
{
    SwitchToTab(tab);
}

void MainView::onTabLocationChanged(ViewTab* tab)
{
    if (text) {
        text->setPlainText(tab->location);
    }
}

void MainView::SwitchToTab(ViewTab* tab)
{
    if (!tab || tab == mCurrentTab) {
        return;
    }
    if (mCurrentTab && !tiledView) {
        lView->removeItem(mCurrentTab);
        scene()->removeItem(mCurrentTab);
        mCurrentTab->SetIsActive(false);
    }
    lView->insertItem(1, tab);
    mCurrentTab = tab;
    text->setPlainText(tab->location);
    if (mCurrentTab) {
        mCurrentTab->SetIsActive(true);
    }
}

void MainView::onButtonOpenClicked()
{
    ViewTab* tab = NULL;
    if (!getenv("LOAD_SAME_TAB") || tabsList.empty()) {
        tab = new ViewTab(mContext, boundingRect().toRect().size());
        connect(tab, SIGNAL(tabButtonClicked(ViewTab*)), this, SLOT(onTabButtonClicked(ViewTab*)));
        connect(tab, SIGNAL(tabLocationChanged(ViewTab*)), this, SLOT(onTabLocationChanged(ViewTab*)));
        tabsList.append(tab);
        lTabsTitleBar->addItem(tab->button());
        SwitchToTab(tab);
    } else {
        tab = tabsList.first();
    }
    if (argIDX < 0) {
        return;
    }
    tab->LoadURL(urls.at(argIDX++));
    if (argIDX == urls.size()) {
        argIDX = 0;
        noLoadUrls = true;
    }
}

void MainView::onButtonReloadClicked()
{
    mCurrentTab->Reload();
}

void MainView::onButtonCloseClicked()
{
    if (mCurrentTab) {
        lView->removeItem(mCurrentTab);
        scene()->removeItem(mCurrentTab);
        lTabsTitleBar->removeItem(mCurrentTab->button());
        scene()->removeItem(mCurrentTab->button());
        tabsList.removeAll(mCurrentTab);
        delete mCurrentTab;
        mCurrentTab = 0;
    }
    if (!tabsList.empty()) {
        SwitchToTab(tabsList.front());
    }
}

void MainView::onContextFinalized()
{
    qApp->quit();
}

void MainView::onButtonQuitClicked()
{
    mContext->Quit();
}

void MainView::CreateLayout()
{
    // start anchor layout
    if (noLayout) {
        setFlag(QGraphicsItem::ItemHasNoContents);
        ViewTab* tab = new ViewTab(mContext, boundingRect().toRect().size());
        tabsList.append(tab);
        scene()->addItem(tab);
        if (argIDX < 0) {
            return;
        }
        tab->LoadURL(urls.at(argIDX++));
        return;
    }
    lMainVertLayout = new QGraphicsLinearLayout(Qt::Vertical);
    lMainVertLayout->setContentsMargins(0, 0, 0, 0);
    lMainVertLayout->setSpacing(0);

    // setup the main widget
    QPalette p;
    p.setColor(QPalette::Window, Qt::blue);
    setPalette(p);
    setPos(0, 0);
    setLayout(lMainVertLayout);

    // Navigation controls
    lHorNavButtonBar = new QGraphicsLinearLayout(Qt::Horizontal);
    lHorNavButtonBar->setContentsMargins(0, 0, 0, 0);
    lHorNavButtonBar->setSpacing(0);
    lHorNavButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

    lTabsViewContainer = new QGraphicsLinearLayout(Qt::Horizontal);
    lTabsViewContainer->setContentsMargins(0, 0, 0, 0);
    lTabsViewContainer->setSpacing(0);

    // Tabs bar
    lTabsTitleBar = new QGraphicsLinearLayout(Qt::Vertical);
    lTabsTitleBar->setContentsMargins(0, 0, 0, 0);
    lTabsTitleBar->setSpacing(0);
    lTabsTitleBar->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    lTabsTitleBar->setMaximumSize(50, 600);

    // Renderer and VKB view
    lViewVKBCont = new QGraphicsLinearLayout(Qt::Vertical);
    lViewVKBCont->setContentsMargins(0, 0, 0, 0);
    lViewVKBCont->setSpacing(0);
    lViewVKBCont->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));


    // Renderer widget view
    lView = new QGraphicsLinearLayout(Qt::Vertical);
    lView->setContentsMargins(0, 0, 0, 0);
    lView->setSpacing(0);
    if (showVkb) {
        lView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        // VKB widget view
        lVkbView = new QGraphicsLinearLayout(Qt::Vertical);
        lVkbView->setContentsMargins(0, 0, 0, 0);
        lVkbView->setSpacing(0);
        lVkbView->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
        lVkbView->setMaximumSize(750, 220);
    } else {
        lView->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding));
        lView->setMinimumSize(750, 220);
    }

    if (showBars) {
        lTabsViewContainer->addItem(lTabsTitleBar);
        lMainVertLayout->addItem(lHorNavButtonBar);
    }
    lViewVKBCont->addItem(lView);

    lTabsViewContainer->addItem(lViewVKBCont);
    lMainVertLayout->addItem(lTabsViewContainer);

    NavButton* bt = new NavButton(QString("Open"), QSize(100, 50));
    connect(bt, SIGNAL(buttonClicked()), this, SLOT(onButtonOpenClicked()));
    lHorNavButtonBar->addItem(bt);

    bt = new NavButton(QString("Reload"), QSize(100, 50));
    connect(bt, SIGNAL(buttonClicked()), this, SLOT(onButtonReloadClicked()));
    lHorNavButtonBar->addItem(bt);

    bt = new NavButton(QString("Close"), QSize(100, 50));
    connect(bt, SIGNAL(buttonClicked()), this, SLOT(onButtonCloseClicked()));
    lHorNavButtonBar->addItem(bt);
    bt = new NavButton(QString("Quit"), QSize(100, 50));
    connect(bt, SIGNAL(buttonClicked()), this, SLOT(onButtonQuitClicked()));
    lHorNavButtonBar->addItem(bt);

    text = new TextItem(QString("DefaultText"));
    lHorNavButtonBar->addItem(text);
}

void
MainView::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    static bool firstPaint = true;
    if (firstPaint) {
        firstPaint = false;
    }
}
