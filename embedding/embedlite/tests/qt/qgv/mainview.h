/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QGraphicsWidget>

class QGraphicsLinearLayout;
class TextItem;
class EmbedContext;
class ViewTab;
class MainView : public QGraphicsWidget
{
    Q_OBJECT
public:
    explicit MainView(EmbedContext* aContext = 0);

    void CreateLayout();
    void SetMozContext(EmbedContext* aContext);

signals:
    void onCloseEvent();

public slots:
    void onTabButtonClicked(ViewTab* tab);
    void onTabLocationChanged(ViewTab* tab);
    void onButtonOpenClicked();
    void onButtonReloadClicked();
    void onButtonCloseClicked();
    void onButtonQuitClicked();
    void onContextFinalized();
    QGraphicsWidget* onRequestNewWindow(QGraphicsWidget* aParent, int flags);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    virtual void closeEvent(QCloseEvent* aEvent);

private:
    void SwitchToTab(ViewTab*);

    EmbedContext* mContext;
    QGraphicsLinearLayout *lMainVertLayout;
    QGraphicsLinearLayout *lHorNavButtonBar;
    QGraphicsLinearLayout *lTabsViewContainer;
    QGraphicsLinearLayout *lTabsTitleBar;
    QGraphicsLinearLayout *lView;
    QGraphicsLinearLayout *lVkbView;
    QGraphicsLinearLayout *lViewVKBCont;
    TextItem* text;
    int argIDX;
    ViewTab* mCurrentTab;
    QList<ViewTab*> tabsList;
    QList<QString> urls;
};

#endif // MAINVIEW_H
