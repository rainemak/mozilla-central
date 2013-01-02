/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qgraphicsmozview_h
#define qgraphicsmozview_h

#include <QGraphicsWidget>

class QMozContext;
class QGraphicsMozViewPrivate;

class QGraphicsMozView : public QGraphicsWidget
{
    Q_OBJECT

public:
    QGraphicsMozView(QGraphicsItem* parent = 0);

    virtual ~QGraphicsMozView();

    virtual void setGeometry(const QRectF& rect);
    virtual void updateGeometry();

    virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint) const;

protected:
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget* widget = 0);

private:
    QGraphicsMozViewPrivate* d;
    friend class QGraphicsMozViewPrivate;
};

#endif /* qgraphicsmozview_h */
