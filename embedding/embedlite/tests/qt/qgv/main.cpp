/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef USE_MEEGO
#include <MApplication>
#endif
#include <QApplication>
#include <QGraphicsView>
#include "embedcontext.h"
#include "graphicsview.h"
#include "mainview.h"
#if defined(Q_WS_X11)
#include <X11/Xlib.h>
#endif

int main(int argc, char *argv[])
{
#if defined(Q_WS_X11)
#if QT_VERSION >= 0x040800
    QApplication::setAttribute(Qt::AA_X11InitThreads, true);
#else
    XInitThreads();
    QApplication::setAttribute(static_cast<Qt::ApplicationAttribute>(10), true);
#endif
#endif
#ifdef USE_MEEGO
    MApplication app(argc, argv);
#else
    QApplication app(argc, argv);
#endif
    app.setQuitOnLastWindowClosed(false);

    QGraphicsScene scene;
    EmbedContext mozcontext(argc, argv);
    QObject::connect(&app, SIGNAL(lastWindowClosed()), &mozcontext, SLOT(onLastWindowClosed()));
    mozcontext.Init();

    MainView window;
    window.SetMozContext(&mozcontext);

    MyGraphicsView view(&scene, &window);
    if (!getenv("SWRENDER")) {
        mozcontext.GetApp()->SetIsAccelerated(true);
        view.SetGLWidget();
    }
    scene.addItem(&window);
    window.CreateLayout();

    // QGV setup
    scene.setBackgroundBrush(Qt::white);
    // Hack
    printf("Show Main View\n");
#ifdef __arm__
    view.showFullScreen();
#else
    view.showNormal();
#endif

    printf("Exec Qt application\n");
    int val = app.exec();
    printf("Exiting from application with val:%i\n", val);
    return val;
}
