/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TextItem_H
#define TextItem_H

#include <QGraphicsWidget>

class TextItem : public QGraphicsTextItem,
                 public QGraphicsWidget
{
public:
    enum { Type = UserType + 2 };

    TextItem(const QString &text = QString(), QGraphicsItem *parent = 0);
    ~TextItem();

    int type() const {
        return Type;
    }

    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
    void setGeometry(const QRectF &rect);
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    virtual void keyPressEvent ( QKeyEvent * event);
    virtual void keyReleaseEvent ( QKeyEvent * event);
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );
    virtual void focusInEvent ( QFocusEvent * event );
    virtual void focusOutEvent ( QFocusEvent * event );

};

#endif // TextItem_H
