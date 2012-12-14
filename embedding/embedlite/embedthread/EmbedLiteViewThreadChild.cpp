/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteViewThreadChild"

#include "EmbedLiteViewThreadChild.h"
#include "EmbedLog.h"
#include "mozilla/unused.h"

#include "nsEmbedCID.h"
#include "nsIBaseWindow.h"
#include "EmbedLitePuppetWidget.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMWindow.h"
#include "nsNetUtil.h"
#include "nsIDocShell.h"

namespace mozilla {
namespace embedlite {

EmbedLiteViewThreadChild::EmbedLiteViewThreadChild(uint32_t aId)
  : mId(aId)
{
    LOGT();
    AddRef();
    MessageLoop::current()->
        PostTask(FROM_HERE,
                 NewRunnableMethod(this,
                                   &EmbedLiteViewThreadChild::InitGeckoWindow));
}

EmbedLiteViewThreadChild::~EmbedLiteViewThreadChild()
{
    LOGT();
}

void
EmbedLiteViewThreadChild::ActorDestroy(ActorDestroyReason aWhy)
{
    LOGT("reason:%i", aWhy);
    mBChrome->RemoveEventHandler();
}

bool EmbedLiteViewThreadChild::RecvDestroy()
{
    LOGT("destroy");
    PEmbedLiteViewChild::Send__delete__(this);
    return true;
}

void
EmbedLiteViewThreadChild::InitGeckoWindow()
{
    LOGT();
    nsresult rv;
    mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return;

    nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebBrowser, &rv);
    if (NS_FAILED(rv))
        return;

    mWidget = new EmbedLitePuppetWidget(this, mId);

    nsWidgetInitData  widgetInit;
    widgetInit.clipChildren = true;
    widgetInit.mWindowType = eWindowType_toplevel;
    mWidget->Create(
        nullptr, 0,              // no parents
        nsIntRect(nsIntPoint(0, 0), nsIntSize(800, 600)),
        nullptr,                 // HandleWidgetEvent
        &widgetInit              // nsDeviceContext
    );

    if (!mWidget) {
        NS_ERROR("couldn't create fake widget");
        return;
    }


    rv = baseWindow->InitWindow(0, mWidget, 0, 0, 800, 600);
    if (NS_FAILED(rv))
        return;

    nsCOMPtr<nsIDOMWindow> domWindow;

    nsIWebBrowserChrome **aNewWindow = getter_AddRefs(mChrome);
    mBChrome = new WebBrowserChrome();
    CallQueryInterface(static_cast<nsIWebBrowserChrome*>(mBChrome), aNewWindow);
    uint32_t aChromeFlags = 0; // View()->GetWindowFlags();

    mWebBrowser->SetContainerWindow(mChrome);

    mChrome->SetChromeFlags(aChromeFlags);
    if (aChromeFlags & (nsIWebBrowserChrome::CHROME_OPENAS_CHROME |
                        nsIWebBrowserChrome::CHROME_OPENAS_DIALOG)) {
        nsCOMPtr<nsIDocShellTreeItem> docShellItem(do_QueryInterface(baseWindow));
        docShellItem->SetItemType(nsIDocShellTreeItem::typeChromeWrapper);
        LOGT("Chrome window created\n");
    }

    if (NS_FAILED(baseWindow->Create())) {
        NS_ERROR("Creation of basewindow failed.\n");
    }

    if (NS_FAILED(mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow)))) {
        NS_ERROR("Failed to get the content DOM window.\n");
    }

    mDOMWindow = do_QueryInterface(domWindow);
    if (!mDOMWindow) {
        NS_ERROR("Got stuck with DOMWindow1!");
    }

    mWebNavigation = do_QueryInterface(baseWindow);
    if (!mWebNavigation) {
        NS_ERROR("Failed to get the web navigation interface.");
    }

    mChrome->SetWebBrowser(mWebBrowser);

    rv = baseWindow->SetVisibility(true);
    if (NS_FAILED(rv)) {
        NS_ERROR("SetVisibility failed.\n");
    }

    unused << SendInitialized();
}

bool
EmbedLiteViewThreadChild::RecvLoadURL(const nsString& url)
{
    LOGT("url:%s", NS_ConvertUTF16toUTF8(url).get());
    nsCOMPtr<nsIIOService> ioService = do_GetService(NS_IOSERVICE_CONTRACTID);
    if (!ioService)
        return true;

    ioService->SetOffline(false);
    if (mWebNavigation) {
        mWebNavigation->LoadURI(url.get(),
                                nsIWebNavigation::LOAD_FLAGS_NONE,
                                0, 0, 0);
    }

    return true;
}

} // namespace embedlite
} // namespace mozilla

