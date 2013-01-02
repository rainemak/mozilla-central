/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "textitem.h"
#include <QTextDocument>
#include <stdio.h>

TextItem::TextItem(const QString &text, QGraphicsItem* parent)
    : QGraphicsTextItem(text, parent)
    , QGraphicsWidget(parent)
{
    QGraphicsWidget::setAcceptHoverEvents(true);
    QGraphicsWidget::setAcceptTouchEvents(true);
    QGraphicsWidget::setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);

    QGraphicsWidget::grabGesture(Qt::PanGesture);
    QGraphicsWidget::grabGesture(Qt::PinchGesture);
    QGraphicsWidget::grabGesture(Qt::TapAndHoldGesture);

    QGraphicsWidget::setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    QGraphicsWidget::setFlag(QGraphicsItem::ItemIsFocusScope, true);
    QGraphicsWidget::setFlag(QGraphicsItem::ItemIsFocusable, true);
    setTextInteractionFlags(Qt::TextEditable);
    setPlainText(text);
}

TextItem::~TextItem()
{
}

void TextItem::keyPressEvent ( QKeyEvent * event)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}
void TextItem::keyReleaseEvent ( QKeyEvent * event)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}
void TextItem::mousePressEvent ( QGraphicsSceneMouseEvent * event )
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void TextItem::mouseReleaseEvent ( QGraphicsSceneMouseEvent * event )
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void TextItem::focusInEvent ( QFocusEvent * event )
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void TextItem::focusOutEvent ( QFocusEvent * event )
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

QSizeF TextItem::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    switch (which) {
    case Qt::MinimumSize:
        return QSizeF(0, 0);
    case Qt::PreferredSize:
        return document()->size();
    case Qt::MaximumSize:
        return QSizeF((1<<24)-1, (1<<24)-1);
    default:
        qWarning("r::TextItem::sizeHint(): Don't know how to handle the value of 'which'");
        break;
    }
    return constraint;
}

void TextItem::setGeometry(const QRectF &rect)
{
    setTextWidth(rect.width());
    QGraphicsWidget::setPos(rect.topLeft());
//  setPos(rect.topLeft());
}

void
TextItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    QGraphicsTextItem::paint(painter, option, widget);
}
