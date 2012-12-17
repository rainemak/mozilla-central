/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/embedlite/EmbedInitGlue.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "embedLiteCoreInitPref.h"

#ifdef MOZ_WIDGET_QT
#include <QApplication>
#elif defined(MOZ_WIDGET_GTK2)
#include <glib/glib.h>
#endif

using namespace mozilla::embedlite;

static bool sDoExit = getenv("NORMAL_EXIT");

class MyListener : public EmbedLiteAppListener, public EmbedLiteViewListener
{
public:
    MyListener(EmbedLiteApp* aApp) : mApp(aApp), mView(NULL) {}
    virtual ~MyListener()
    {
    }
    virtual void Initialized()
    {
        printf("Embedding initialized, let's make view");
        InitDefaultEmbedBrowserPrefs(mApp);
        mView = mApp->CreateView();
        mView->SetListener(this);
    }
    virtual void ViewInitialized()
    {
        printf("Embedding has created view:%p, Yay\n", mView);
        mView->LoadURL("data:text/html,<body bgcolor=red>TestApp</body>");
    }
    virtual bool Invalidate()
    {
        printf("Embedding Has something for render\n");
        mApp->PostTask(&MyListener::RenderImage, this);
        return true;
    }
    static void RenderImage(void* aData)
    {
        MyListener* self = static_cast<MyListener*>(aData);
        printf("Embedding render Image\n");
    }

private:
    EmbedLiteApp* mApp;
    EmbedLiteView* mView;
};

int main(int argc, char** argv)
{
#ifdef MOZ_WIDGET_QT
    QApplication app(argc, argv);
#elif defined(MOZ_WIDGET_GTK2)
    g_thread_init(NULL);
#endif

    printf("Load XUL Symbols\n");
    if (LoadEmbedLite(argc, argv)) {
        printf("XUL Symbols loaded\n");
        EmbedLiteApp* mapp = XRE_GetEmbedLite();
        MyListener* listener = new MyListener(mapp);
        mapp->SetListener(listener);
        bool res = mapp->Start(EmbedLiteApp::EMBED_THREAD);
        printf("XUL Symbols loaded: init res:%i\n", res);
        delete listener;
        delete mapp;
    } else {
        printf("XUL Symbols failed to load\n");
    }
    UnloadEmbedLite();
    return 0;
}
