/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NAVBUTTON_H
#define NAVBUTTON_H

#include <QGraphicsWidget>

class NavButton : public QGraphicsWidget
{
    Q_OBJECT
public:
    explicit NavButton(QString buttonText, QSize size);
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    void SetText(QString aText) {
        m_buttonText = aText;
        update();
    }
    void SetOrientationAngle(qreal aAngle) {
        m_angle = aAngle;
        update();
    }

signals:
    void buttonClicked();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    QString m_buttonText;
    QSize m_size;
    qreal m_angle;
};

#endif // NAVBUTTON_H
