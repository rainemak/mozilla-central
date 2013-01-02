/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "navbutton.h"
#include <QPainter>

NavButton::NavButton(QString buttonText, QSize size)
    : QGraphicsWidget()
    , m_buttonText(buttonText)
    , m_size(size)
    , m_angle(0)
{
}

QRectF NavButton::boundingRect() const
{
    return QRectF(QPoint(0,0), m_size);
}

void NavButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // printf("NavButton::mousePressEvent [%g,%g]\n", event->scenePos().x(), event->scenePos().y());
}

void NavButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // printf("NavButton::mouseReleaseEvent [%g,%g]\n", event->scenePos().x(), event->scenePos().y());
    emit buttonClicked();
}

void NavButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPen paintPen = QPen();
    QFont textFont = QFont("Helvetica [Cronyx]", 12, QFont::Normal);
    float borderWidth = 5.0f;
    QRectF textRect(borderWidth/2, borderWidth/2, m_size.width() - borderWidth, m_size.height() - borderWidth);
    paintPen.setWidth(borderWidth);
    paintPen.setColor(QColor(Qt::red));
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(paintPen);
    painter->setFont(textFont);
    painter->drawRoundedRect(textRect, 10, 10);
    if (m_angle) {
        //painter->translate(textRect.width() / 2, textRect.height() / 2);
        painter->translate(m_size.width() / 2 + 5, m_size.height() - 10);
        painter->rotate(m_angle);
        painter->drawText(0, 0, m_buttonText);
    } else {
        painter->drawText(textRect, Qt::AlignCenter, m_buttonText);
    }
    painter->restore();
}
