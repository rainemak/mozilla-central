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

    virtual void resizeEvent(QGraphicsSceneResizeEvent*);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent*);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent*);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
    virtual void dragEnterEvent(QGraphicsSceneDragDropEvent*);
    virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent*);
    virtual void dragMoveEvent(QGraphicsSceneDragDropEvent*);
    virtual void dropEvent(QGraphicsSceneDragDropEvent*);
    virtual void focusInEvent(QFocusEvent*);
    virtual void focusOutEvent(QFocusEvent*);

signals:
    void tabButtonClicked(ViewTab*);
    void tabLocationChanged(ViewTab*);

public slots:
    void onContextInitialized();
    void onButtonClicked();
    void on_titleChanged(QString);
    void on_locationChanged(QString);
    void on_pageShowHide(QString, bool);
    void on_loadFinished(bool);
    void on_firstPaint(int,int);
    void on_Observe(const QString topic, QString data);

private:
    void ViewInitialized();
    void Destroyed();
    bool Invalidate();
    void SetupGLViewPort();
    NavButton* mButton;
    bool mIsActive;
    mozilla::embedlite::EmbedLiteView* mView;
    EmbedContext* mContext;
    bool mInitialized;
    QString pendingUrl;
    QTime mPanningTime;
    QSize mSize;
    QImage image;
};

#endif // VIEWTAB_H
