/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "graphicsview.h"
#include <QGraphicsWidget>

MyGraphicsView::MyGraphicsView(QGraphicsScene* scene, QGraphicsWidget* widget)
    : QGraphicsView(scene), mTopLevel(widget), mGL(0)
{
    setFocusPolicy(Qt::StrongFocus);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::TapAndHoldGesture);
}

void MyGraphicsView::SetGLWidget()
{
    if (!mGL) {
        mGL = new QGLWidget(QGLFormat(QGL::SampleBuffers));
    }
    setViewport(mGL);
    mGL->makeCurrent();
}

void MyGraphicsView::resizeEvent(QResizeEvent *event)
{
    printf(">>>>>>Func:%s::%d evSz[%i,%i]\n", __PRETTY_FUNCTION__, __LINE__, event->size().width(), event->size().height());
    mTopLevel->setGeometry(0, 0, event->size().width(), event->size().height());
    setSceneRect(0, 0, event->size().width(), event->size().height());
}

void MyGraphicsView::paintEvent(QPaintEvent* ev)
{
    // Qt does not check context for current in GL paint engine, let's force it as current here
    if (mGL) {
        mGL->makeCurrent();
    }
    static MozFPSCounter counter("SHOW_FPS", "QGVFPS", 30);
    counter.Count();
    QGraphicsView::paintEvent(ev);
}
