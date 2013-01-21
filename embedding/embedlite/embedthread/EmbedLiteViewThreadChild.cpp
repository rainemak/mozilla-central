/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset:4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteViewThreadChild"
#include "EmbedLog.h"

#include "EmbedLiteViewThreadChild.h"
#include "EmbedLiteAppThreadChild.h"
#include "EmbedLiteViewScrolling.h"

#include "mozilla/unused.h"

#include "nsEmbedCID.h"
#include "nsIBaseWindow.h"
#include "EmbedLitePuppetWidget.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMWindow.h"
#include "nsNetUtil.h"
#include "nsIDocShell.h"
#include "nsIFocusManager.h"
#include "nsFocusManager.h"

#include "nsIDOMWindowUtils.h"
#include "nsPIDOMWindow.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "mozilla/layers/AsyncPanZoomController.h"
#include "nsIScriptSecurityManager.h"
#include "mozilla/Preferences.h"
#include "EmbedPromptService.h"

using namespace mozilla::layers;
using namespace mozilla::widget;

namespace mozilla {
namespace embedlite {

EmbedLiteViewThreadChild::EmbedLiteViewThreadChild(uint32_t aId)
  : mId(aId)
  , mViewSize(0, 0)
  , mScrolling(new EmbedLiteViewScrolling(this))
  , mDispatchSynthMouseEvents(true)
  , mHadResizeSinceLastFrameUpdate(false)
  , mModalDepth(0)
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
    EmbedLiteAppThreadChild::GetInstance()->ModulesService()->UnregisterView(this);
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
    nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebBrowser);
    docShell->SetIsBrowserInsideApp(nsIScriptSecurityManager::NO_APP_ID);

    mChrome->SetWebBrowser(mWebBrowser);

    rv = baseWindow->SetVisibility(true);
    if (NS_FAILED(rv)) {
        NS_ERROR("SetVisibility failed.\n");
    }

    EmbedLiteAppThreadChild::GetInstance()->ModulesService()->RegisterView(this);

    mHelper = new TabChildHelper(this);
    unused << SendInitialized();
}

bool
EmbedLiteViewThreadChild::RecvLoadURL(const nsString& url)
{
    LOGT("url:%s", NS_ConvertUTF16toUTF8(url).get());
    NS_ENSURE_TRUE(mWebNavigation, false);

    nsCOMPtr<nsIIOService> ioService = do_GetService(NS_IOSERVICE_CONTRACTID);
    if (!ioService)
        return true;

    ioService->SetOffline(false);
    mWebNavigation->LoadURI(url.get(),
                            nsIWebNavigation::LOAD_FLAGS_NONE,
                            0, 0, 0);

    return true;
}

bool
EmbedLiteViewThreadChild::RecvGoBack()
{
    NS_ENSURE_TRUE(mWebNavigation, false);

    mWebNavigation->GoBack();
    return true;
}

bool
EmbedLiteViewThreadChild::RecvGoForward()
{
    NS_ENSURE_TRUE(mWebNavigation, false);

    mWebNavigation->GoForward();
    return true;
}

bool
EmbedLiteViewThreadChild::RecvStopLoad()
{
    NS_ENSURE_TRUE(mWebNavigation, false);

    mWebNavigation->Stop(nsIWebNavigation::STOP_NETWORK);
    return true;
}

bool
EmbedLiteViewThreadChild::RecvReload(const bool& aHardReload)
{
    NS_ENSURE_TRUE(mWebNavigation, false);
    uint32_t reloadFlags = aHardReload ?
      nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY | nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE :
      nsIWebNavigation::LOAD_FLAGS_NONE;

    mWebNavigation->Reload(reloadFlags);
    return true;
}

bool
EmbedLiteViewThreadChild::RecvSetIsActive(const bool& aIsActive)
{
    if (!mWebBrowser || !mDOMWindow)
        return false;
    nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
    NS_ENSURE_TRUE(fm, false);
    if (aIsActive) {
        fm->WindowRaised(mDOMWindow);
        LOGT("Activate browser");
    } else {
        fm->WindowLowered(mDOMWindow);
        LOGT("Deactivate browser");
    }
    mWebBrowser->SetIsActive(aIsActive);
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
    mHadResizeSinceLastFrameUpdate = true;

    return true;
}

bool
EmbedLiteViewThreadChild::RecvAsyncScrollDOMEvent(const gfxRect& contentRect,
                                                  const gfxSize& scrollSize)
{
    mScrolling->AsyncScrollDOMEvent(contentRect, scrollSize);
    return true;
}

bool
EmbedLiteViewThreadChild::RecvUpdateFrame(const FrameMetrics& aFrameMetrics)
{
    if (!mWebBrowser)
        return true;

    bool ret = mHelper->RecvUpdateFrame(aFrameMetrics);
    const InputContext& ctx = mWidget->GetInputContext();
    if (ctx.mIMEState.mEnabled && mHadResizeSinceLastFrameUpdate) {
        mScrolling->ScrollToFocusedInput(false);
    }
    mHadResizeSinceLastFrameUpdate = false;

    return ret;
}

bool
EmbedLiteViewThreadChild::RecvHandleDoubleTap(const nsIntPoint& aPoint)
{
    if (!mWebBrowser)
        return true;

    nsString data;
    data.AppendPrintf("{ \"x\" : %d", aPoint.x);
    data.AppendPrintf(", \"y\" : %d", aPoint.y);
    data.AppendPrintf(" }");

    if (getenv("LOAD_BR_CHILD"))
        mHelper->RecvAsyncMessage(NS_LITERAL_STRING("Gesture:DoubleTap"), data);
    else
        mHelper->RecvHandleDoubleTap(aPoint);

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
EmbedLiteViewThreadChild::RecvHandleTextEvent(const nsString& commit, const nsString& preEdit)
{
    nsPoint offset;
    nsCOMPtr<nsIWidget> widget = mHelper->GetWidget(&offset);
    if (!widget)
        return false;

    {
        nsCompositionEvent event(true, NS_COMPOSITION_START, widget);
        mHelper->InitEvent(event, nullptr);
        event.data = commit;
        mHelper->DispatchWidgetEvent(event);
    }

    {
        nsCompositionEvent event(true, NS_COMPOSITION_UPDATE, widget);
        mHelper->InitEvent(event, nullptr);
        event.data = commit;
        mHelper->DispatchWidgetEvent(event);
    }

    {
        nsTextEvent event(true, NS_TEXT_TEXT, widget);
        mHelper->InitEvent(event, nullptr);
        event.theText = commit;
        mHelper->DispatchWidgetEvent(event);
    }

    {
        nsCompositionEvent event(true, NS_COMPOSITION_END, widget);
        mHelper->InitEvent(event, nullptr);
        event.data = commit;
        mHelper->DispatchWidgetEvent(event);
    }

    return true;
}

bool
EmbedLiteViewThreadChild::RecvHandleKeyPressEvent(const int& domKeyCode, const int& gmodifiers, const int& charCode)
{
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mWebNavigation);
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);
    NS_ENSURE_TRUE(utils, true);
    bool handled = false;
    // If the key isn't autorepeat, we need to send the initial down event
    utils->SendKeyEvent(NS_LITERAL_STRING("keydown"), domKeyCode, charCode, gmodifiers, 0, &handled);
    utils->SendKeyEvent(NS_LITERAL_STRING("keypress"), domKeyCode, charCode, gmodifiers, 0, &handled);
    return true;
}

bool
EmbedLiteViewThreadChild::RecvHandleKeyReleaseEvent(const int& domKeyCode, const int& gmodifiers, const int& charCode)
{
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mWebNavigation);
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);
    NS_ENSURE_TRUE(utils, true);
    bool handled = false;
    utils->SendKeyEvent(NS_LITERAL_STRING("keyup"), domKeyCode, charCode, gmodifiers, 0, &handled);
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

bool
EmbedLiteViewThreadChild::RecvInputDataTouchEvent(const mozilla::MultiTouchInput& aData, const gfxSize& res, const gfxPoint& diff)
{
    nsTouchEvent localEvent;
    if (mHelper->ConvertMutiTouchInputToEvent(aData, res, diff, localEvent)) {
        nsEventStatus status =
            mHelper->DispatchWidgetEvent(localEvent);
        nsCOMPtr<nsPIDOMWindow> outerWindow = do_GetInterface(mWebNavigation);
        nsCOMPtr<nsPIDOMWindow> innerWindow = outerWindow->GetCurrentInnerWindow();
        if (innerWindow && innerWindow->HasTouchEventListeners()) {
            SendContentReceivedTouch(nsIPresShell::gPreventMouseEvents);
            if (status == nsEventStatus_eConsumeNoDefault) {
                SendCancelDefaultPanZoom();
            }
        }
        static bool sDispatchMouseEvents;
        static bool sDispatchMouseEventsCached = false;
        if (!sDispatchMouseEventsCached) {
            sDispatchMouseEventsCached = true;
            Preferences::AddBoolVarCache(&sDispatchMouseEvents,
                "embedlite.dispatch_mouse_events", false);
        }
        if (status != nsEventStatus_eConsumeNoDefault && mDispatchSynthMouseEvents && sDispatchMouseEvents) {
            // Touch event not handled
            status = mHelper->DispatchSynthesizedMouseEvent(localEvent);
            if (status != nsEventStatus_eConsumeNoDefault && status != nsEventStatus_eConsumeDoDefault) {
                mDispatchSynthMouseEvents = false;
            } else {
                SendCancelDefaultPanZoom();
            }
        }
    }
    if (aData.mType == MultiTouchInput::MULTITOUCH_END ||
        aData.mType == MultiTouchInput::MULTITOUCH_CANCEL ||
        aData.mType == MultiTouchInput::MULTITOUCH_LEAVE) {
        mDispatchSynthMouseEvents = true;
    }
    return true;
}

bool
EmbedLiteViewThreadChild::RecvInputDataTouchMoveEvent(const mozilla::MultiTouchInput& aData, const gfxSize& res, const gfxPoint& diff)
{
    return RecvInputDataTouchEvent(aData, res, diff);
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
    nsresult rv = NS_OK;
    nsCOMPtr <nsIDOMWindow> window;
    rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(window));

    nsCOMPtr<nsIDOMDocument> ddoc;
    window->GetDocument(getter_AddRefs(ddoc));
    NS_ENSURE_TRUE(ddoc, NS_OK);

    nsCOMPtr<nsIDocument> doc;
    doc = do_QueryInterface(ddoc, &rv);
    NS_ENSURE_TRUE(doc, NS_OK);

    nsIPresShell *presShell = doc->GetShell();
    if (presShell) {
        nscolor bgcolor = presShell->GetCanvasBackground();
        unused << SendSetBackgroundColor(bgcolor);
    }

    unused << RecvSetViewSize(mViewSize);

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

NS_IMETHODIMP EmbedLiteViewThreadChild::OnUpdateDisplayPort()
{
    LOGNI();
    return NS_OK;
}

void
EmbedLiteViewThreadChild::WaitForPromptResult(EmbedLiteViewPromptResponse* resp)
{
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(resp->mWin);
    NS_ENSURE_TRUE(utils, );

    uint64_t outerWindowID = 0, innerWindowID = 0;
    utils->GetOuterWindowID(&outerWindowID);
    utils->GetCurrentInnerWindowID(&innerWindowID);
    if (innerWindowID == 0) {
        // I have no idea what waiting for a result means when there's no inner
        // window, so let's just bail.
        NS_WARNING("_waitForResult: No inner window. Bailing.");
        return;
    }

    modalWinMap[outerWindowID] = resp;

    nsCOMPtr<nsIDOMWindow> modalStateWin;
    nsresult rv = utils->EnterModalStateWithWindow(getter_AddRefs(modalStateWin));
    // We'll decrement win.modalDepth when we receive a UnblockPrompt message
    // for the window.
    mModalDepth++;
    int origModalDepth = mModalDepth;

    // process events until we're finished.
    nsIThread *thread = NS_GetCurrentThread();
    while (mModalDepth == origModalDepth && NS_SUCCEEDED(rv)) {
        bool processedEvent;
        rv = thread->ProcessNextEvent(true, &processedEvent);
        if (NS_SUCCEEDED(rv) && !processedEvent) {
            rv = NS_ERROR_UNEXPECTED;
        }
        nsCOMPtr<nsIDOMWindowUtils> tutils = do_GetInterface(resp->mWin);
        uint64_t innerWindowIDTemp = 0;
        tutils->GetCurrentInnerWindowID(&innerWindowIDTemp);
        if (innerWindowIDTemp != innerWindowID) {
            NS_WARNING("_waitForResult: Inner window ID changed while in nested event loop.");
            rv = NS_ERROR_UNEXPECTED;
        }
    }
    utils->LeaveModalStateWithWindow(modalStateWin);
}

bool
EmbedLiteViewThreadChild::RecvUnblockPrompt(const uint64_t& winID,
                                            const bool& checkValue,
                                            const bool& confirm,
                                            const nsString& retValue,
                                            const nsString& username,
                                            const nsString& password)
{
    EmbedLiteViewPromptResponse* resp = modalWinMap[winID];

    if (!resp) {
        NS_ERROR("RecvUnblockPrompt, but winID is not registered\n");
        return false;
    }

    modalWinMap.erase(winID);

    if (!resp->mWin) {
      NS_ERROR("RecvUnblockPrompt, but window is gone\n");
      return false;
    }

    resp->checkvalue = checkValue;
    resp->confirm = confirm;
    resp->retVal = retValue;
    resp->username = username;
    resp->password = password;

    mModalDepth--;
    return true;
}

} // namespace embedlite
} // namespace mozilla

