/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QGraphicsMozView"

#include <QStyleOptionGraphicsItem>
#include <QPainter>

#include "mozilla-config.h"
#include "qgraphicsmozview.h"
#include "mozilla/embedlite/EmbedLog.h"

class QGraphicsMozViewPrivate {
public:
    QGraphicsMozViewPrivate(QGraphicsMozView* view)
      : q(view)
    {
    }

    QGraphicsMozView* q;
};

QGraphicsMozView::QGraphicsMozView(QGraphicsItem* parent)
    : QGraphicsWidget(parent)
    , d(new QGraphicsMozViewPrivate(this))
{
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setFocusPolicy(Qt::StrongFocus);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
}

QGraphicsMozView::~QGraphicsMozView()
{
    delete d;
}

void
QGraphicsMozView::paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget*)
{
    QRect r = opt ? opt->exposedRect.toRect() : boundingRect().toRect();
    LOGT("r[%i,%i,%i,%i]", r.x(), r.y(), r.width(), r.height());
    painter->fillRect(r, Qt::red);
}

/*! \reimp
*/
QSizeF QGraphicsMozView::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
{
    if (which == Qt::PreferredSize)
        return QSizeF(800, 600); // ###
    return QGraphicsWidget::sizeHint(which, constraint);
}

/*! \reimp
*/
void QGraphicsMozView::updateGeometry()
{
    QGraphicsWidget::updateGeometry();

    QSize size = geometry().size().toSize();
    LOGT("Page Update: [%i,%i]", size.width(), size.height());
}

/*! \reimp
*/
void QGraphicsMozView::setGeometry(const QRectF& rect)
{
    QGraphicsWidget::setGeometry(rect);

    // NOTE: call geometry() as setGeometry ensures that
    // the geometry is within legal bounds (minimumSize, maximumSize)
    QSize size = geometry().size().toSize();
    LOGT("Page Update: [%i,%i]", size.width(), size.height());
}
