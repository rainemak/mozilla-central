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
#include "InputData.h"

using namespace mozilla;
using namespace mozilla::embedlite;

ViewTab::ViewTab(EmbedContext* aContext, QSize aSize, int flags, QGraphicsWidget* aParent)
    : QGraphicsWidget(aParent)
    , mButton(new NavButton(QString("Tab"), QSize(50, 100)))
    , mIsActive(true)
    , mView(NULL)
    , mContext(aContext)
    , mInitialized(false)
    , mPendingTouchEvent(false)
{
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
    connect(aContext, SIGNAL(geckoContextInitialized()), this, SLOT(onContextInitialized()));
    if (aContext->IsInitialized()) {
        onContextInitialized();
    }
}

ViewTab::~ViewTab()
{
    if (mView) {
        mContext->GetApp()->DestroyView(mView);
    }
}

QGraphicsWidget*
ViewTab::button()
{
    return mButton;
}

void ViewTab::onContextInitialized()
{
    mView = mContext->GetApp()->CreateView();
    mView->SetListener(this);
    mView->SetScrollingMode(true);
}

void ViewTab::SetupGLViewPort()
{
    if (scene() && scene()->views().size() == 1) {
        QSize sz(scene()->views()[0]->viewport()->width(), scene()->views()[0]->viewport()->height());
        mView->SetGLViewPortSize(sz.width(), sz.height());
    }
}

void ViewTab::ViewInitialized()
{
    mInitialized = true;
    mView->SetViewSize(mSize.width(), mSize.height());
    SetupGLViewPort();
    if (!pendingUrl.isEmpty()) {
        LoadURL(pendingUrl);
    }
    mView->LoadFrameScript("chrome://global/content/embedTestScript.js");
    mView->SendAsyncMessage("EmbedMsg::HelloChildScript", "{}");
}

void
ViewTab::RecvAsyncMessage(const char* aMessage, const char* aData)
{
    printf(">>>>>>Func:%s::%d msg:%s, data:%s\n", __PRETTY_FUNCTION__, __LINE__, aMessage, aData);
}

char*
ViewTab::RecvSyncMessage(const char* aMessage, const char* aData)
{
    printf(">>>>>>Func:%s::%d msg:%s, data:%s\n", __PRETTY_FUNCTION__, __LINE__, aMessage, aData);
    return "{\"id\": \"test\", \"val\": \"5\"}";
}

void ViewTab::Destroyed()
{
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
    emit tabButtonClicked(this);
}

void ViewTab::resizeEvent(QGraphicsSceneResizeEvent* ev)
{
    mSize = ev->newSize().toSize();
    if (mInitialized) {
        mView->SetViewSize(mSize.width(), mSize.height());
        SetupGLViewPort();
    }
}

void ViewTab::paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget* widget)
{
    QRect r = opt ? opt->exposedRect.toRect() : boundingRect().toRect();
    if (mInitialized) {
        QMatrix affine = painter->transform().toAffine();
        gfxMatrix matr(affine.m11(), affine.m12(), affine.m21(), affine.m22(), affine.dx(), affine.dy());
        mView->SetGLViewTransform(matr);
        if (mContext->GetApp()->IsAccelerated()) {
            mView->RenderGL();
        } else {
            if (mTempBufferImage.isNull() || mTempBufferImage.width() != r.width() || mTempBufferImage.height() != r.height()) {
                mTempBufferImage = QImage(r.size(), QImage::Format_RGB16);
            }
            mTempBufferImage.fill(Qt::white);
            mView->RenderToImage(mTempBufferImage.bits(), mTempBufferImage.width(),
                                 mTempBufferImage.height(), mTempBufferImage.bytesPerLine(),
                                 mTempBufferImage.depth());
            painter->drawImage(QPoint(0, 0), mTempBufferImage);
        }
    }
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
        printf(">>>>>>Func:%s::%d Event Show\n", __PRETTY_FUNCTION__, __LINE__);
        break;
    case QEvent::Hide:
        printf(">>>>>>Func:%s::%d Event Hide\n", __PRETTY_FUNCTION__, __LINE__);
        break;
    default:
        break;
    }

    // Here so that it can be reimplemented without breaking ABI.
    return QGraphicsWidget::event(event);
}

void ViewTab::touchEvent(QTouchEvent* event)
{
    mPendingTouchEvent = event->type() == QEvent::TouchEnd ? false : true;
    // Always accept the QTouchEvent so that we'll receive also TouchUpdate and TouchEnd events
    event->setAccepted(true);

    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_START, mTouchTime.elapsed());
    MultiTouchInput meventMove(MultiTouchInput::MULTITOUCH_MOVE, mTouchTime.elapsed());
    MultiTouchInput meventEnd(MultiTouchInput::MULTITOUCH_END, mTouchTime.elapsed());
    for (int i = 0; i < event->touchPoints().size(); ++i) {
        const QTouchEvent::TouchPoint& pt = event->touchPoints().at(i);
        nsIntPoint nspt(pt.pos().x(), pt.pos().y());
        switch (pt.state())
        {
            case Qt::TouchPointPressed: {
                meventStart.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                   nspt,
                                                   nsIntPoint(1, 1),
                                                   180.0f,
                                                   1.0f));
                break;
            }
            case Qt::TouchPointReleased: {
                meventEnd.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                 nspt,
                                                 nsIntPoint(1, 1),
                                                 180.0f,
                                                 1.0f));
                break;
            }
            case Qt::TouchPointMoved: {
                meventMove.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                  nspt,
                                                  nsIntPoint(1, 1),
                                                  180.0f,
                                                  1.0f));
                break;
            }
            default:
                break;
        }
    }
    if (meventStart.mTouches.Length())
        mView->ReceiveInputEvent(meventStart);
    if (meventMove.mTouches.Length())
        mView->ReceiveInputEvent(meventMove);
    if (meventEnd.mTouches.Length())
        mView->ReceiveInputEvent(meventEnd);
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
}

void ViewTab::SetIsActive(bool aIsActive, bool aForce)
{
    mIsActive = aIsActive;
}

void ViewTab::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if (mInitialized && !mPendingTouchEvent) {
        const bool accepted = e->isAccepted();
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_MOVE, mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(e->pos().x(), e->pos().y()),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        mView->ReceiveInputEvent(event);
        e->setAccepted(accepted);
    }

    if (!e->isAccepted())
        QGraphicsItem::mouseMoveEvent(e);
}

void ViewTab::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    mPanningTime.restart();
    if (mInitialized && !mPendingTouchEvent) {
        const bool accepted = e->isAccepted();
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_START, mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(e->pos().x(), e->pos().y()),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        mView->ReceiveInputEvent(event);
        e->setAccepted(accepted);
    }

    if (!e->isAccepted())
        QGraphicsItem::mouseMoveEvent(e);
}

void ViewTab::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    if (mInitialized && !mPendingTouchEvent) {
        const bool accepted = e->isAccepted();
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_END, mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(e->pos().x(), e->pos().y()),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        mView->ReceiveInputEvent(event);
        e->setAccepted(accepted);
    }

    if (!e->isAccepted())
        QGraphicsItem::mouseMoveEvent(e);
}
