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
//        mView->LoadURL("data:text/html,<body bgcolor=red>TestApp</body>");
        mView->LoadURL("http://ya.ru");
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
        char* data = self->mView->GetImageAsURL(800, 600);
        data[25] = 0;
        printf("Embedding render Image: %s\n", data);
        delete data;
    }
    virtual void OnTitleChanged(const PRUnichar* aTitle)
    {
        printf("OnTitleChanged: title:%s\n", NS_ConvertUTF16toUTF8(aTitle).get());
    }
    virtual void OnLocationChanged(const char* aLocation, bool aCanGoBack, bool aCanGoForward)
    {
        printf("OnLocationChanged: loc:%s, canBack:%i, canForw:%i\n", aLocation, aCanGoBack, aCanGoForward);
    }
    virtual void OnLoadStarted(const char* aLocation)
    {
        printf("OnLoadStarted: loc:%s\n", aLocation);
    }
    virtual void OnLoadFinished(void)
    {
        printf("OnLoadFinished\n");
    }
    virtual void OnLoadRedirect(void)
    {
        printf("OnLoadRedirect\n");
    }
    virtual void OnLoadProgress(int32_t aProgress)
    {
        printf("OnLoadProgress: progress:%i\n", aProgress);
    }
    virtual void OnSecurityChanged(const char* aStatus, unsigned int aState)
    {
        printf("OnSecurityChanged: status:%s, stat:%u\n", aStatus, aState);
    }
    virtual void OnFirstPaint(int32_t aX, int32_t aY)
    {
        printf("OnFirstPaint pos[%i,%i]\n", aX, aY);
    }
    virtual void OnContentLoaded(const PRUnichar* aDocURI)
    {
        printf("OnContentLoaded: docURI:%s\n", NS_ConvertUTF16toUTF8(aDocURI).get());
    }
    virtual void OnLinkAdded(const PRUnichar* aHref, const PRUnichar* aCharset, const PRUnichar* aTitle, const PRUnichar* aRel, const PRUnichar* aSizes, const PRUnichar* aType)
    {
        printf("OnLinkAdded: href:%s, chars:%s, title:%s, rel:%s, sizes:%s, type:%s\n",
               NS_ConvertUTF16toUTF8(aHref).get(), NS_ConvertUTF16toUTF8(aCharset).get(),
               NS_ConvertUTF16toUTF8(aTitle).get(), NS_ConvertUTF16toUTF8(aRel).get(), NS_ConvertUTF16toUTF8(aSizes).get(), NS_ConvertUTF16toUTF8(aType).get());
    }
    virtual void OnWindowOpenClose(const PRUnichar* aType)
    {
        printf("OnWindowOpenClose: type:%s\n", NS_ConvertUTF16toUTF8(aType).get());
    }
    virtual void OnPopupBlocked(const char* aSpec, const char* aCharset, const PRUnichar* aPopupFeatures, const PRUnichar* aPopupWinName)
    {
        printf("OnPopupBlocked spec:%s, chars:%s, popFeat:%s, popWinName:%s\n", aSpec, aCharset, NS_ConvertUTF16toUTF8(aPopupFeatures).get(), NS_ConvertUTF16toUTF8(aPopupWinName).get());
    }
    virtual void OnPageShowHide(const PRUnichar* aType, bool aPersisted)
    {
        printf("OnPageShowHide: type:%s, pers:%i\n", NS_ConvertUTF16toUTF8(aType).get(), aPersisted);
    }
    virtual void OnScrolledAreaChanged(unsigned int aWidth, unsigned int aHeight)
    {
        printf("OnScrolledAreaChanged: sz[%u,%u]\n", aWidth, aHeight);
    }
    virtual void OnScrollChanged(int32_t offSetX, int32_t offSetY)
    {
        printf("OnScrollChanged: scroll:%i,%i\n", offSetX, offSetY);
    }
    virtual void OnObserve(const char* aTopic, const PRUnichar* aData)
    {
        printf("OnObserve: top:%s, data:%s\n", aTopic, NS_ConvertUTF16toUTF8(aData).get());
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
