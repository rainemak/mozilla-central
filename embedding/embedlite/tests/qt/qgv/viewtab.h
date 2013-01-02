/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef VIEWTAB_H
#define VIEWTAB_H

#include <QGraphicsWidget>
#include <QTime>
#include <QImage>
#include "mozilla/embedlite/EmbedLiteView.h"
#include "InputData.h"

class EmbedContext;
class NavButton;
class ViewTab : public QGraphicsWidget,
                public mozilla::embedlite::EmbedLiteViewListener
{
    Q_OBJECT
public:
    ViewTab(EmbedContext* aContext, QSize aSize, int flags = 0, QGraphicsWidget* aParent = 0);
    virtual ~ViewTab();
    QGraphicsWidget* button();
    void LoadURL(QString aUrl);
    void Reload();
    virtual void SetIsActive(bool aIsActive, bool aForce = false);

    QString title;
    QString location;

protected:
    virtual void wheelEvent(QGraphicsSceneWheelEvent*);
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget* widget = 0);
    virtual bool event(QEvent*);

    virtual void resizeEvent(QGraphicsSceneResizeEvent*);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*);

signals:
    void tabButtonClicked(ViewTab*);
    void tabLocationChanged(ViewTab*);

public slots:
    void onContextInitialized();
    void onButtonClicked();

private:
    void ReceiveInputEvent(const mozilla::InputData& event);
    void touchEvent(QTouchEvent* event);
    void ViewInitialized();
    void Destroyed();
    bool Invalidate();
    void RecvAsyncMessage(const char* aMessage, const char* aData);
    char* RecvSyncMessage(const char* aMessage, const char* aData);
    void SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    void SetupGLViewPort();
    NavButton* mButton;
    bool mIsActive;
    mozilla::embedlite::EmbedLiteView* mView;
    EmbedContext* mContext;
    bool mInitialized;
    QString pendingUrl;
    QTime mPanningTime;
    QTime mTouchTime;
    QSize mSize;
    QImage mTempBufferImage;
    bool mPendingTouchEvent;
    QColor mBgColor;
};

#endif // VIEWTAB_H
