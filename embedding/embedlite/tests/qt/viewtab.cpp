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
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPinchGesture>
#include "gfxMatrix.h"

ViewTab::ViewTab(EmbedContext* aContext, QSize aSize, int flags, QGraphicsWidget* aParent)
    : QGraphicsWidget(aParent)
    , mButton(new NavButton(QString("Tab"), QSize(50, 100)))
    , mIsActive(true)
    , mView(NULL)
    , mContext(aContext)
    , mInitialized(false)
{
    printf(">>>>>>Func:%s::%d, sz[%i,%i]\n", __PRETTY_FUNCTION__, __LINE__, aSize.width(), aSize.height());
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);

    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::TapAndHoldGesture);

    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    setFlag(QGraphicsItem::ItemIsFocusScope, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);

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
        onContextInitialized();
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

void ViewTab::SetupGLViewPort()
{
    if (scene() && scene()->views().size() == 1) {
        QSize sz(scene()->views()[0]->viewport()->width(), scene()->views()[0]->viewport()->height());
        printf(">>>>>>Func:%s::%d rect[%i,%i]\n", __PRETTY_FUNCTION__, __LINE__, sz.width(), sz.height());        
        mView->SetGLViewPortSize(sz.width(), sz.height());
    }
}

void ViewTab::ViewInitialized()
{
    mInitialized = true;
    printf(">>>>>>Func:%s::%d url:%s\n", __PRETTY_FUNCTION__, __LINE__, pendingUrl.toUtf8().data());
    mView->SetViewSize(mSize.width(), mSize.height());
    SetupGLViewPort();
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

bool ViewTab::event(QEvent* event)
{
    QEvent::Type eventType = event->type();
    switch (eventType) {
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
        touchEvent(static_cast<QTouchEvent*>(event));
        return true;
    case QEvent::Show:
//        page()->d->page->drawingArea()->setPageIsVisible(true);
        printf(">>>>>>Func:%s::%d Event Show\n", __PRETTY_FUNCTION__, __LINE__);
        break;
    case QEvent::Hide:
        printf(">>>>>>Func:%s::%d Event Hide\n", __PRETTY_FUNCTION__, __LINE__);
//        page()->d->page->drawingArea()->setPageIsVisible(false);
        break;
    case QEvent::Gesture:
        gestureEvent(static_cast<QGestureEvent*>(event));
        break;
    default:
        break;
    }

    // Here so that it can be reimplemented without breaking ABI.
    return QGraphicsWidget::event(event);
}

void ViewTab::gestureEvent(QGestureEvent* event)
{
    // TODO: we could remove all gesture handling here
    // if no parent register to any gestures.
    QGesture* gesture = event->gesture(Qt::PinchGesture);
    if (gesture) {
        QPinchGesture* pinch = static_cast<QPinchGesture*>(gesture);
        QPoint center = mapFromScene(pinch->centerPoint()).toPoint();
        if (pinch->state() == Qt::GestureStarted) {
            mView->PinchStart(center.x(), center.y());
        }
        if (pinch->state() == Qt::GestureUpdated) {
            mView->PinchUpdate(center.x(), center.y(), pinch->scaleFactor());
        }
        if (pinch->state() == Qt::GestureFinished) {
            mView->PinchEnd(center.x(), center.y(), pinch->scaleFactor());
        }
    }
    // We eat all gesture events, we deal with them ourself.
    QList<QGesture*> gestures = event->gestures();
    for (int i = 0; i < gestures.size(); ++i) {
        event->accept(gestures.at(i));
    }
}

void ViewTab::touchEvent(QTouchEvent* event)
{
    printf(">>>>>>Func:%s::%d Touch Event: type:%i, touchLen:%i\n", __PRETTY_FUNCTION__, __LINE__, event->type(), event->touchPoints().length());
}

void ViewTab::paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget* widget)
{
    QRect r = opt ? opt->exposedRect.toRect() : boundingRect().toRect();
    bool pt = false;
    painter->fillRect(r, pt ? Qt::red : Qt::blue);
    pt = !pt;
    if (mInitialized) {
        QMatrix affine = painter->transform().toAffine();
        gfxMatrix matr(affine.m11(), affine.m12(), affine.m21(), affine.m22(), affine.dx(), affine.dy());
        mView->SetTransform(matr);
        if (mContext->GetApp()->IsAccelerated()) {
            mView->RenderGL();
        } else {
            if (image.isNull() || image.width() != r.width() || image.height() != r.height()) {
                image = QImage(r.size(), QImage::Format_RGB16);
            }
            mView->RenderToImage(image.bits(), image.width(), image.height(), image.bytesPerLine(), image.depth());
            painter->drawImage(QPoint(0, 0), image);
        }
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
        SetupGLViewPort();
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
