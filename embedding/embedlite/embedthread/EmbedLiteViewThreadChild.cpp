/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset:4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteViewThreadChild"
#include "EmbedLog.h"

#include "EmbedLiteViewThreadChild.h"
#include "EmbedLiteAppThreadChild.h"

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
#include "EmbedLiteAppService.h"

using namespace mozilla::layers;
using namespace mozilla::widget;

namespace mozilla {
namespace embedlite {

EmbedLiteViewThreadChild::EmbedLiteViewThreadChild(uint32_t aId)
  : mId(aId)
  , mOuterId(0)
  , mViewSize(0, 0)
  , mDispatchSynthMouseEvents(true)
  , mIMEComposing(false)
{
    LOGT();
    AddRef();
    MessageLoop::current()->
        PostTask(FROM_HERE,
                 NewRunnableMethod(this,
                                   &EmbedLiteViewThreadChild::InitGeckoWindow));
    mRegisteredMessages.Init();
}

EmbedLiteViewThreadChild::~EmbedLiteViewThreadChild()
{
    LOGT();
    NS_ASSERTION(mControllerListeners.IsEmpty(), "Controller listeners list is not empty...");
}

EmbedLiteAppThreadChild*
EmbedLiteViewThreadChild::AppChild()
{
  return EmbedLiteAppThreadChild::GetInstance();
}

void
EmbedLiteViewThreadChild::ActorDestroy(ActorDestroyReason aWhy)
{
    LOGT("reason:%i", aWhy);
    mHelper->Disconnect();
    mControllerListeners.Clear();
}

bool EmbedLiteViewThreadChild::RecvDestroy()
{
    LOGT("destroy");
    mControllerListeners.Clear();
    AppChild()->AppService()->UnregisterView(mId);
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

    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(mDOMWindow);
    utils->GetOuterWindowID(&mOuterId);

    AppChild()->AppService()->RegisterView(mId);

    nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
    if (observerService)
        observerService->NotifyObservers(mDOMWindow, "embedliteviewcreated", nullptr);

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

    mHelper = new TabChildHelper(this);
    unused << SendInitialized();
}

void
EmbedLiteViewThreadChild::GetBrowser(nsIWebBrowser** outBrowser)
{
    NS_ADDREF(*outBrowser = mWebBrowser.get());
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
EmbedLiteViewThreadChild::RecvSuspendTimeouts()
{
    if (!mDOMWindow)
        return false;

    nsresult rv;
    nsCOMPtr<nsPIDOMWindow> pwindow(do_QueryInterface(mDOMWindow, &rv));
    NS_ENSURE_SUCCESS(rv, false);
    if (!pwindow->TimeoutSuspendCount())
        pwindow->SuspendTimeouts();

    return true;
}

bool
EmbedLiteViewThreadChild::RecvResumeTimeouts()
{
    if (!mDOMWindow)
        return false;

    nsresult rv;
    nsCOMPtr<nsPIDOMWindow> pwindow(do_QueryInterface(mDOMWindow, &rv));
    NS_ENSURE_SUCCESS(rv, false);

    rv = pwindow->ResumeTimeouts();
    NS_ENSURE_SUCCESS(rv, false);

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
    AppChild()->AppService()->HandleAsyncMessage(NS_ConvertUTF16toUTF8(aMessage).get(), aData);
    mHelper->RecvAsyncMessage(aMessage, aData);
    return true;
}

bool
EmbedLiteViewThreadChild::HasMessageListener(const nsAString& aMessageName)
{
    if (mRegisteredMessages.Get(aMessageName)) {
        return true;
    }
    return false;
}

bool
EmbedLiteViewThreadChild::DoSendAsyncMessage(const PRUnichar* aMessageName, const PRUnichar* aMessage)
{
    LOGT("msg:%s, data:%s", NS_ConvertUTF16toUTF8(aMessageName).get(), NS_ConvertUTF16toUTF8(aMessage).get());
    if (mRegisteredMessages.Get(nsDependentString(aMessageName))) {
        return SendAsyncMessage(nsDependentString(aMessageName), nsDependentString(aMessage));
    }
    return true;
}

bool
EmbedLiteViewThreadChild::DoSendSyncMessage(const PRUnichar* aMessageName, const PRUnichar* aMessage, InfallibleTArray<nsString>* aJSONRetVal)
{
    LOGT("msg:%s, data:%s", NS_ConvertUTF16toUTF8(aMessageName).get(), NS_ConvertUTF16toUTF8(aMessage).get());
    if (mRegisteredMessages.Get(nsDependentString(aMessageName))) {
        return SendSyncMessage(nsDependentString(aMessageName), nsDependentString(aMessage), aJSONRetVal);
    }
    return true;
}

void
EmbedLiteViewThreadChild::RecvAsyncMessage(const nsAString& aMessage,
                                           const nsAString& aData)
{
    LOGT("msg:%s, data:%s", NS_ConvertUTF16toUTF8(aMessage).get(), NS_ConvertUTF16toUTF8(aData).get());
    mHelper->RecvAsyncMessage(aMessage, aData);
}

bool
EmbedLiteViewThreadChild::RecvAddMessageListener(const nsCString& name)
{
    LOGT("name:%s", name.get());
    mRegisteredMessages.Put(NS_ConvertUTF8toUTF16(name), 1);
    return true;
}

bool
EmbedLiteViewThreadChild::RecvRemoveMessageListener(const nsCString& name)
{
    LOGT("name:%s", name.get());
    mRegisteredMessages.Remove(NS_ConvertUTF8toUTF16(name));
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

void
EmbedLiteViewThreadChild::AddGeckoContentListener(mozilla::layers::GeckoContentController *listener)
{
    mControllerListeners.AppendElement(listener);
}

void
EmbedLiteViewThreadChild::RemoveGeckoContentListener(mozilla::layers::GeckoContentController *listener)
{
    mControllerListeners.RemoveElement(listener);
}

bool
EmbedLiteViewThreadChild::RecvAsyncScrollDOMEvent(const gfxRect& contentRect,
                                                  const gfxSize& scrollSize)
{
    gfx::Rect rect(contentRect.x, contentRect.y, contentRect.width, contentRect.height);
    gfx::Size size(scrollSize.width, scrollSize.height);
    for (unsigned int i = 0; i < mControllerListeners.Length(); i++) {
        mControllerListeners[i]->SendAsyncScrollDOMEvent(rect, size);
    }

    return true;
}

bool
EmbedLiteViewThreadChild::RecvUpdateFrame(const FrameMetrics& aFrameMetrics)
{
    if (!mWebBrowser)
        return true;

    for (unsigned int i = 0; i < mControllerListeners.Length(); i++) {
        mControllerListeners[i]->RequestContentRepaint(aFrameMetrics);
    }

    bool ret = mHelper->RecvUpdateFrame(aFrameMetrics);

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

    for (unsigned int i = 0; i < mControllerListeners.Length(); i++) {
        mControllerListeners[i]->HandleDoubleTap(aPoint);
    }

    if (getenv("LOAD_BR_CHILD"))
        mHelper->RecvAsyncMessage(NS_LITERAL_STRING("Gesture:DoubleTap"), data);

    return true;
}

bool
EmbedLiteViewThreadChild::RecvHandleSingleTap(const nsIntPoint& aPoint)
{
    for (unsigned int i = 0; i < mControllerListeners.Length(); i++) {
        mControllerListeners[i]->HandleSingleTap(aPoint);
    }

    RecvMouseEvent(NS_LITERAL_STRING("mousemove"), aPoint.x, aPoint.y, 0, 1, 0, false);
    RecvMouseEvent(NS_LITERAL_STRING("mousedown"), aPoint.x, aPoint.y, 0, 1, 0, false);
    RecvMouseEvent(NS_LITERAL_STRING("mouseup"), aPoint.x, aPoint.y, 0, 1, 0, false);

    return true;
}

bool
EmbedLiteViewThreadChild::RecvHandleLongTap(const nsIntPoint& aPoint)
{
    for (unsigned int i = 0; i < mControllerListeners.Length(); i++) {
        mControllerListeners[i]->HandleLongTap(aPoint);
    }

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
    const InputContext& ctx = mWidget->GetInputContext();

    LOGF("ctx.mIMEState.mEnabled:%i, com:%s, pre:%s\n", ctx.mIMEState.mEnabled, NS_ConvertUTF16toUTF8(commit).get(), NS_ConvertUTF16toUTF8(preEdit).get());
    if (!widget || !ctx.mIMEState.mEnabled)
        return false;

    // probably logic here is over engineered, but clean enough
    bool prevIsComposition = mIMEComposing;
    bool StartComposite = !prevIsComposition && commit.IsEmpty() && !preEdit.IsEmpty();
    bool UpdateComposite = prevIsComposition && commit.IsEmpty() && !preEdit.IsEmpty();
    bool EndComposite = prevIsComposition && !commit.IsEmpty() && preEdit.IsEmpty();
    mIMEComposing = UpdateComposite || StartComposite;
    nsString pushStr = preEdit.IsEmpty() ? commit : preEdit;
    if (!commit.IsEmpty() && !EndComposite) {
        StartComposite = UpdateComposite = EndComposite = true;
    }

    if (StartComposite)
    {
        nsCompositionEvent event(true, NS_COMPOSITION_START, widget);
        mHelper->InitEvent(event, nullptr);
        mHelper->DispatchWidgetEvent(event);
    }

    if (StartComposite || UpdateComposite)
    {
        nsCompositionEvent event(true, NS_COMPOSITION_UPDATE, widget);
        mHelper->InitEvent(event, nullptr);
        mHelper->DispatchWidgetEvent(event);
    }

    if (StartComposite || UpdateComposite || EndComposite)
    {
        nsTextEvent event(true, NS_TEXT_TEXT, widget);
        mHelper->InitEvent(event, nullptr);
        event.theText = pushStr;
        mHelper->DispatchWidgetEvent(event);
    }

    if (EndComposite)
    {
        nsCompositionEvent event(true, NS_COMPOSITION_END, widget);
        mHelper->InitEvent(event, nullptr);
        mHelper->DispatchWidgetEvent(event);
    }

    return true;
}

void EmbedLiteViewThreadChild::ResetInputState()
{
    if (!mIMEComposing)
        return;

    mIMEComposing = false;
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

NS_IMETHODIMP EmbedLiteViewThreadChild::OnUpdateDisplayPort()
{
    LOGNI();
    return NS_OK;
}

} // namespace embedlite
} // namespace mozilla

