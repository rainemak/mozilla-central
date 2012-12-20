/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "viewtab.h"
#include "navbutton.h"
#include "embedcontext.h"
#include <QPainter>
#include <QGraphicsSceneWheelEvent>
#include <QStyleOptionGraphicsItem>

ViewTab::ViewTab(EmbedContext* aContext, QSize aSize, int flags, QGraphicsWidget* aParent)
    : QGraphicsWidget(aParent)
    , mButton(new NavButton(QString("Tab"), QSize(50, 100)))
    , mIsActive(true)
    , mView(NULL)
    , mContext(aContext)
    , mInitialized(false)
{
    printf(">>>>>>Func:%s::%d, sz[%i,%i]\n", __PRETTY_FUNCTION__, __LINE__, aSize.width(), aSize.height());
    setPreferredSize(aSize.width(), aSize.height());
    mSize = aSize;
    mButton->SetOrientationAngle(270);
    connect(mButton, SIGNAL(buttonClicked()), this, SLOT(onButtonClicked()));
    connect(this, SIGNAL(locationChanged(QString)), this, SLOT(on_locationChanged(QString)));
    connect(this, SIGNAL(titleChanged(QString)), this, SLOT(on_titleChanged(QString)));
    connect(this, SIGNAL(pageShowHide(QString,bool)), this, SLOT(on_pageShowHide(QString,bool)));
    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(on_loadFinished(bool)));
    connect(this, SIGNAL(firstPaint(int,int)), this, SLOT(on_firstPaint(int,int)));
    connect(this, SIGNAL(Observe(const QString,QString)), this, SLOT(on_Observe(const QString,QString)));
    connect(aContext, SIGNAL(geckoContextInitialized()), this, SLOT(onContextInitialized()));
    if (aContext->IsInitialized()) {
        mView = mContext->GetApp()->CreateView();
        mView->SetListener(this);
        mView->SetScrollingMode(true);
    }
//  AddObserver("ime-enabled-state-changed");
}

ViewTab::~ViewTab()
{
    if (mView) {
        mContext->GetApp()->DestroyView(mView);
    }
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

QGraphicsWidget*
ViewTab::button()
{
    return mButton;
}

void ViewTab::onContextInitialized()
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    mView = mContext->GetApp()->CreateView();
    mView->SetListener(this);
    mView->SetScrollingMode(true);
}

void ViewTab::ViewInitialized()
{
    mInitialized = true;
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    mView->SetViewSize(mSize.width(), mSize.height());
    if (!pendingUrl.isEmpty()) {
        LoadURL(pendingUrl);
    }
}

void ViewTab::Destroyed()
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    mView = NULL;
    mInitialized = false;
}

bool ViewTab::Invalidate()
{
    update();
    return true;
}

void ViewTab::LoadURL(QString aUrl)
{
    if (mInitialized) {
        mView->LoadURL(aUrl.toUtf8().data());
    } else {
        pendingUrl = aUrl;
    }
    location = aUrl;
}

void ViewTab::Reload()
{
    LoadURL(location);
}

void ViewTab::onButtonClicked()
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    emit tabButtonClicked(this);
}

void ViewTab::on_pageShowHide(QString url, bool show)
{
    printf(">>>>>>Func:%s::%d url:%s, show:%i\n", __PRETTY_FUNCTION__, __LINE__, url.toUtf8().data(), show);
}

void ViewTab::on_titleChanged(QString atitle)
{
    printf(">>>>>>Func:%s::%d, title:%s\n", __PRETTY_FUNCTION__, __LINE__, title.toUtf8().data());
    title = atitle;
    mButton->SetText(title);
}

void ViewTab::on_locationChanged(QString aurl)
{
    printf(">>>>>>Func:%s::%d, url:%s\n", __PRETTY_FUNCTION__, __LINE__, aurl.toUtf8().data());
    location = aurl;
    emit tabLocationChanged(this);
}

void ViewTab::paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget* widget)
{
    QRect r = opt ? opt->exposedRect.toRect() : boundingRect().toRect();
    painter->fillRect(r, Qt::red);
    if (mInitialized) {
        if (image.isNull() || image.width() != r.width() || image.height() != r.height()) {
            image = QImage(r.size(), QImage::Format_ARGB32);
        }
        mView->RenderToImage(image.bits(), image.width(), image.height(), image.bytesPerLine(), image.depth());
        painter->drawImage(QPoint(0, 0), image);
    }
}

void ViewTab::wheelEvent(QGraphicsSceneWheelEvent* aEvent)
{
    static const int WHEEL_DELTA = 120;
    int delta = (aEvent->delta() / WHEEL_DELTA) * -10;
    QPoint diff(0, 0);
    switch (aEvent->orientation()) {
    case Qt::Vertical:
        diff.setY(delta);
        break;
    case Qt::Horizontal:
        diff.setX(delta);
        break;
    default:
        Q_ASSERT(0);
        break;
    }
    //ScrollBy(diff.x(), diff.y());
    //UpdateViewport();
}

void ViewTab::on_loadFinished(bool)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
//  if (mIsActive && !(GetWindowFlags() & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)) {
//    ObserveNotification("current-embedded-active-window", "");
//  }
}

void ViewTab::on_firstPaint(int,int)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
//  if (mIsActive && !(GetWindowFlags() & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)) {
//    ObserveNotification("current-embedded-active-window", "");
//  }
}

void ViewTab::SetIsActive(bool aIsActive, bool aForce)
{
    mIsActive = aIsActive;
//  QMozEmbedQGVWidget::SetIsActive(aIsActive, aForce);
}

void ViewTab::on_Observe(const QString topic, QString data)
{
    printf(">>>>>>Func:%s::%d top:%s, data:%s\n", __PRETTY_FUNCTION__, __LINE__, topic.toUtf8().data(), data.toUtf8().data());
}

void ViewTab::resizeEvent(QGraphicsSceneResizeEvent* ev)
{
    printf(">>>>>>Func:%s::%d sz[%g,%g]\n", __PRETTY_FUNCTION__, __LINE__, ev->newSize().width(), ev->newSize().height());
    mSize = ev->newSize().toSize();
    if (mInitialized) {
        mView->SetViewSize(mSize.width(), mSize.height());
    }
}

void ViewTab::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
//    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    if (mInitialized) {
        mView->MouseMove(e->pos().x(), e->pos().y(), mPanningTime.elapsed(), e->buttons(), e->modifiers());
    }
}

void ViewTab::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
//    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    mPanningTime.restart();
    if (mInitialized) {
        mView->MousePress(e->pos().x(), e->pos().y(), mPanningTime.elapsed(), e->buttons(), e->modifiers());
    }
}

void ViewTab::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
//    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    if (mInitialized) {
        mView->MouseRelease(e->pos().x(), e->pos().y(), mPanningTime.elapsed(), e->buttons(), e->modifiers());
    }
}

void ViewTab::contextMenuEvent(QGraphicsSceneContextMenuEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::hoverMoveEvent(QGraphicsSceneHoverEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::keyPressEvent(QKeyEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::keyReleaseEvent(QKeyEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::dragEnterEvent(QGraphicsSceneDragDropEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::dragLeaveEvent(QGraphicsSceneDragDropEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::dragMoveEvent(QGraphicsSceneDragDropEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::dropEvent(QGraphicsSceneDragDropEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::focusInEvent(QFocusEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void ViewTab::focusOutEvent(QFocusEvent*)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}
