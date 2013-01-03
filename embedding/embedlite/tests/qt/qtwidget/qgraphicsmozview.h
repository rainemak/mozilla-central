/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qgraphicsmozview_h
#define qgraphicsmozview_h

#include <QGraphicsWidget>
#include <QUrl>

class QMozContext;
class QGraphicsMozViewPrivate;

class QGraphicsMozView : public QGraphicsWidget
{
    Q_OBJECT

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY navigationHistoryChanged FINAL)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY navigationHistoryChanged FINAL)
    Q_PROPERTY(int loadProgress READ loadProgress NOTIFY loadProgressChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL)

public:
    QGraphicsMozView(QGraphicsItem* parent = 0);

    virtual ~QGraphicsMozView();

    QUrl url() const;
    void setUrl(const QUrl&);
    QString title() const;
    int loadProgress() const;
    bool canGoBack() const;
    bool canGoForward() const;
    bool loading() const;

public Q_SLOTS:
    void loadHtml(const QString& html, const QUrl& baseUrl = QUrl());
    void goBack();
    void goForward();
    void stop();
    void reload();

Q_SIGNALS:
    void viewLoaded();
    void urlChanged();
    void titleChanged();
    void loadProgressChanged();
    void navigationHistoryChanged();
    void loadingChanged();

protected:
    virtual void setGeometry(const QRectF& rect);
    virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint) const;

protected:
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget* widget = 0);
    virtual bool event(QEvent*);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*);

private slots:
    void onInitialized();

private:
    QGraphicsMozViewPrivate* d;
    friend class QGraphicsMozViewPrivate;
};

#endif /* qgraphicsmozview_h */
