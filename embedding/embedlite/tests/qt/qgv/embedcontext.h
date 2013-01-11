/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EMBEDCONTEXT_H
#define EMBEDCONTEXT_H

#include <QThread>
#include "mozilla/embedlite/EmbedLiteApp.h"

class EmbedContext;
class QEventLoop;
class GeckoThread : public QThread
{
    Q_OBJECT
public:
    GeckoThread(EmbedContext* aContext) : mContext(aContext), mEventLoop(NULL) {}
    void run();

Q_SIGNALS:
    void geckoSymbolsLoaded();

public Q_SLOTS:
    void Quit();

public:
    EmbedContext* mContext;
    QEventLoop* mEventLoop;
};

class EmbedContext : public QObject,
                     public mozilla::embedlite::EmbedLiteAppListener
{
    Q_OBJECT
public:
    explicit EmbedContext(int, char *argv[], QObject* = 0);
    ~EmbedContext();

    void Init();

    // EmbedLiteAppListener iface
    virtual void Initialized();
    virtual void Destroyed();
    virtual bool ExecuteChildThread();
    virtual bool StopChildThread();

    void Quit();

    mozilla::embedlite::EmbedLiteApp* GetApp() { return mApp; }

    bool IsInitialized() { return mInitialized; }

public Q_SLOTS:
    void contextInitialized();
    void onLastWindowClosed();
    void runEmbedding();

Q_SIGNALS:
    void geckoContextInitialized();
    void contextFinalized();
    void contextFinalizedThread();

protected:

private:
    void setDefaultPrefs();
    mozilla::embedlite::EmbedLiteApp* mApp;
    friend class GeckoThread;
    GeckoThread* mThread;
    bool mInitialized;
};

#endif // EMBEDCONTEXT_H
