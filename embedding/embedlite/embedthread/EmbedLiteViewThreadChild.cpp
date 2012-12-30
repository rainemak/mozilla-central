/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteViewThreadChild"

#include "EmbedLiteViewThreadChild.h"
#include "EmbedLiteViewScrolling.h"
#include "EmbedLog.h"
#include "mozilla/unused.h"

#include "nsEmbedCID.h"
#include "nsIBaseWindow.h"
#include "EmbedLitePuppetWidget.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMWindow.h"
#include "nsNetUtil.h"
#include "nsIDocShell.h"

#include "nsIDOMWindowUtils.h"
#include "nsPIDOMWindow.h"
#include "mozilla/layers/AsyncPanZoomController.h"

using namespace mozilla::layers;

namespace mozilla {
namespace embedlite {

EmbedLiteViewThreadChild::EmbedLiteViewThreadChild(uint32_t aId)
  : mId(aId)
  , mViewSize(0, 0)
  , mScrolling(new EmbedLiteViewScrolling(this))
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
    mHelper->Disconnect();
}

bool EmbedLiteViewThreadChild::RecvDestroy()
{
    LOGT("destroy");
    mHelper->Unload();
    mBChrome->RemoveEventHandler();
    mWidget = nullptr;
    mWebBrowser = nullptr;
    mChrome = nullptr;
    mDOMWindow = nullptr;
    mWebNavigation = nullptr;
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

    rv = baseWindow->InitWindow(0, mWidget, 0, 0, mViewSize.width, mViewSize.height);
    if (NS_FAILED(rv))
        return;

    nsCOMPtr<nsIDOMWindow> domWindow;

    nsIWebBrowserChrome **aNewWindow = getter_AddRefs(mChrome);
    mBChrome = new WebBrowserChrome(this);
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

    mHelper = new TabChildHelper(this);

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

bool
EmbedLiteViewThreadChild::RecvLoadFrameScript(const nsString& uri)
{
    if (mHelper) {
        return mHelper->DoLoadFrameScript(uri);
    }
    return false;
}

bool
EmbedLiteViewThreadChild::RecvAsyncMessage(const nsString& aMessage,
                                           const nsString& aData)
{
    LOGT("msg:%s, data:%s", NS_ConvertUTF16toUTF8(aMessage).get(), NS_ConvertUTF16toUTF8(aData).get());
    mHelper->RecvAsyncMessage(aMessage, aData);
    return true;
}

bool
EmbedLiteViewThreadChild::RecvSetViewSize(const gfxSize& aSize)
{
    mViewSize = aSize;
    LOGT("sz[%g,%g]", mViewSize.width, mViewSize.height);

    if (!mWebBrowser)
        return true;

    nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebBrowser);
    baseWindow->SetPositionAndSize(0, 0, mViewSize.width, mViewSize.height, true);
    baseWindow->SetVisibility(true);
    return true;
}

bool
EmbedLiteViewThreadChild::RecvUpdateFrame(const FrameMetrics& aFrameMetrics)
{
    if (!mWebBrowser)
        return true;

    return mHelper->RecvUpdateFrame(aFrameMetrics);
}

bool
EmbedLiteViewThreadChild::RecvHandleDoubleTap(const nsIntPoint& aPoint)
{
    if (!mWebBrowser)
        return true;

    mScrolling->GestureDoubleTap(aPoint);

    return true;
}

bool
EmbedLiteViewThreadChild::RecvHandleSingleTap(const nsIntPoint& aPoint)
{
    RecvMouseEvent(NS_LITERAL_STRING("mousemove"), aPoint.x, aPoint.y, 0, 1, 0, false);
    RecvMouseEvent(NS_LITERAL_STRING("mousedown"), aPoint.x, aPoint.y, 0, 1, 0, false);
    RecvMouseEvent(NS_LITERAL_STRING("mouseup"), aPoint.x, aPoint.y, 0, 1, 0, false);

    return true;
}

bool
EmbedLiteViewThreadChild::RecvHandleLongTap(const nsIntPoint& aPoint)
{
    RecvMouseEvent(NS_LITERAL_STRING("contextmenu"), aPoint.x, aPoint.y,
                  2 /* Right button */,
                  1 /* Click count */,
                  0 /* Modifiers */,
                  false /* Ignore root scroll frame */);

    return true;
}

bool
EmbedLiteViewThreadChild::RecvMouseEvent(const nsString& aType,
                         const float&    aX,
                         const float&    aY,
                         const int32_t&  aButton,
                         const int32_t&  aClickCount,
                         const int32_t&  aModifiers,
                         const bool&     aIgnoreRootScrollFrame)
{
    if (!mWebBrowser)
        return true;

    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mWebNavigation);
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);

    NS_ENSURE_TRUE(utils, true);
    utils->SendMouseEvent(aType, aX, aY, aButton, aClickCount, aModifiers,
                          aIgnoreRootScrollFrame, 0, 0);
    return true;
}

/* void onTitleChanged (in wstring aTitle) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnTitleChanged(const PRUnichar* aTitle)
{
    return SendOnTitleChanged(nsDependentString(aTitle)) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onLocationChanged (in string aLocation) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnLocationChanged(const char* aLocation, bool aCanGoBack, bool aCanGoForward)
{
    return SendOnLocationChanged(nsDependentCString(aLocation), aCanGoBack, aCanGoForward) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onLoadStarted (in string aLocation) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnLoadStarted(const char* aLocation)
{
    return SendOnLoadStarted(nsDependentCString(aLocation)) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onLoadFinished () */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnLoadFinished()
{
    return SendOnLoadFinished() ? NS_OK : NS_ERROR_FAILURE;
}

/* void onLoadRedirect () */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnLoadRedirect()
{
    return SendOnLoadRedirect() ? NS_OK : NS_ERROR_FAILURE;
}

/* void onLoadProgress (in int32_t aProgress) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnLoadProgress(int32_t aProgress, int32_t aCurTotal, int32_t aMaxTotal)
{
    return SendOnLoadProgress(aProgress, aCurTotal, aMaxTotal) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onSecurityChanged (in string aStatus, in uint32_t aState) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnSecurityChanged(const char* aStatus, uint32_t aState)
{
    return SendOnSecurityChanged(nsDependentCString(aStatus), aState) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onFirstPaint (in int32_t aX, in int32_t aY) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnFirstPaint(int32_t aX, int32_t aY)
{
    return SendOnFirstPaint(aX, aY) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onContentLoaded (in wstring aDocURI) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnContentLoaded(const PRUnichar* aDocURI)
{
    return SendOnContentLoaded(nsDependentString(aDocURI)) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onLinkAdded (in wstring aHref, in wstring aCharset, in wstring aTitle, in wstring aRel, in wstring aSizes, in wstring aType) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnLinkAdded(const PRUnichar* aHref,
                                                    const PRUnichar* aCharset,
                                                    const PRUnichar* aTitle,
                                                    const PRUnichar* aRel,
                                                    const PRUnichar* aSizes,
                                                    const PRUnichar* aType)
{
    return SendOnLinkAdded(
        nsDependentString(aHref),
        nsDependentString(aCharset),
        nsDependentString(aTitle),
        nsDependentString(aRel),
        nsDependentString(aSizes),
        nsDependentString(aType)) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onWindowOpenClose (in wstring aType) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnWindowOpenClose(const PRUnichar* aType)
{
    return SendOnWindowOpenClose(nsDependentString(aType)) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onPopupBlocked (in string aSpec, in string aCharset, in wstring aPopupFeatures, in wstring aPopupWinName) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnPopupBlocked(const char* aSpec, const char* aCharset,
                                                       const PRUnichar* aPopupFeatures,
                                                       const PRUnichar* aPopupWinName)
{
    return SendOnPopupBlocked(
        nsDependentCString(aSpec),
        nsDependentCString(aCharset),
        nsDependentString(aPopupFeatures),
        nsDependentString(aPopupWinName)) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onPageShowHide (in wstring aType, in boolean aPersisted) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnPageShowHide(const PRUnichar* aType, bool aPersisted)
{
    return SendOnPageShowHide(nsDependentString(aType), aPersisted) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onScrolledAreaChanged (in uint32_t aWidth, in uint32_t aHeight) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnScrolledAreaChanged(uint32_t aWidth, uint32_t aHeight)
{
    return SendOnScrolledAreaChanged(aWidth, aHeight) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onScrollChanged (in int32_t offSetX, in int32_t offSetY) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnScrollChanged(int32_t offSetX, int32_t offSetY)
{
    return SendOnScrollChanged(offSetX, offSetY) ? NS_OK : NS_ERROR_FAILURE;
}

/* void onObserve (in string aTopic, in wstring aData) */
NS_IMETHODIMP EmbedLiteViewThreadChild::OnObserve(const char* aTopic, const PRUnichar* aData)
{
     return SendOnObserve(nsDependentCString(aTopic),
                          nsDependentString(aData)) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP EmbedLiteViewThreadChild::OnMetaAdded()
{
    mHelper->HandlePossibleViewportChange();
    return NS_OK;
}

} // namespace embedlite
} // namespace mozilla

