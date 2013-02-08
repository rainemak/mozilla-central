/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "TabChildHelper"
#include "EmbedLog.h"

#include "TabChildHelper.h"
#include "nsIWidget.h"

#include "EmbedTabChildGlobal.h"
#include "EmbedLiteViewThreadChild.h"
#include "mozilla/layers/AsyncPanZoomController.h"

#include "nsNetUtil.h"
#include "nsEventListenerManager.h"
#include "nsIDOMWindowUtils.h"
#include "mozilla/dom/Element.h"
#include "nsIDOMHTMLBodyElement.h"
#include "mozilla/dom/HTMLBodyElement.h"
#include "nsGlobalWindow.h"
#include "nsIDocShell.h"
#include "nsViewportInfo.h"
#include "nsPIWindowRoot.h"
#include "StructuredCloneUtils.h"
#include "mozilla/Preferences.h"

static const char CANCEL_DEFAULT_PAN_ZOOM[] = "cancel-default-pan-zoom";
static const char BROWSER_ZOOM_TO_RECT[] = "browser-zoom-to-rect";
static const char DETECT_SCROLLABLE_SUBFRAME[] = "detect-scrollable-subframe";

using namespace mozilla::embedlite;
using namespace mozilla::layers;
using namespace mozilla::dom;

TabChildHelper::TabChildHelper(EmbedLiteViewThreadChild* aView)
  : mView(aView)
  , mContentDocumentIsDisplayed(false)
  , mTabChildGlobal(nullptr)
{
    LOGT();

    nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);

    if (observerService) {
        observerService->AddObserver(this,
                                     CANCEL_DEFAULT_PAN_ZOOM,
                                     false);
        observerService->AddObserver(this,
                                     BROWSER_ZOOM_TO_RECT,
                                     false);
        observerService->AddObserver(this,
                                     DETECT_SCROLLABLE_SUBFRAME,
                                     false);
    }
    if (!mCx && !InitTabChildGlobal()) {
        NS_WARNING("Failed to register child global ontext");
    }
}

TabChildHelper::~TabChildHelper()
{
    LOGT();
    if (mCx) {
        DestroyCx();
    }

    if (mTabChildGlobal) {
        nsEventListenerManager* elm = mTabChildGlobal->GetListenerManager(false);
        if (elm) {
            elm->Disconnect();
        }
        mTabChildGlobal->mTabChild = nullptr;
    }
}

void
TabChildHelper::Disconnect()
{
    LOGT();
    if (mTabChildGlobal) {
        // The messageManager relays messages via the TabChild which
        // no longer exists.
        static_cast<nsFrameMessageManager*>
            (mTabChildGlobal->mMessageManager.get())->Disconnect();
        mTabChildGlobal->mMessageManager = nullptr;
    }
}

class UnloadScriptEvent : public nsRunnable
{
public:
    UnloadScriptEvent(TabChildHelper* aTabChild, EmbedTabChildGlobal* aTabChildGlobal)
    : mTabChild(aTabChild), mTabChildGlobal(aTabChildGlobal)
    { }

    NS_IMETHOD Run()
    {
        LOGT();
        nsCOMPtr<nsIDOMEvent> event;
        NS_NewDOMEvent(getter_AddRefs(event), nullptr, nullptr);
        if (event) {
            event->InitEvent(NS_LITERAL_STRING("unload"), false, false);
            event->SetTrusted(true);

            bool dummy;
            mTabChildGlobal->DispatchEvent(event, &dummy);
        }

        return NS_OK;
    }

    nsRefPtr<TabChildHelper> mTabChild;
    EmbedTabChildGlobal* mTabChildGlobal;
};

void
TabChildHelper::Unload()
{
    LOGT();
    if (mTabChildGlobal) {
        // Let the frame scripts know the child is being closed
        nsContentUtils::AddScriptRunner(
            new UnloadScriptEvent(this, mTabChildGlobal)
        );
    }
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);

    observerService->RemoveObserver(this, CANCEL_DEFAULT_PAN_ZOOM);
    observerService->RemoveObserver(this, BROWSER_ZOOM_TO_RECT);
    observerService->RemoveObserver(this, DETECT_SCROLLABLE_SUBFRAME);
}

NS_IMPL_ISUPPORTS1(TabChildHelper, nsIObserver)

bool
TabChildHelper::InitTabChildGlobal()
{
    if (mCx && mTabChildGlobal) {
        return true;
    }

    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mView->mWebNavigation);
    NS_ENSURE_TRUE(window, false);
    nsCOMPtr<nsIDOMEventTarget> chromeHandler =
        do_QueryInterface(window->GetChromeEventHandler());
    NS_ENSURE_TRUE(chromeHandler, false);

    nsRefPtr<EmbedTabChildGlobal> scope = new EmbedTabChildGlobal(this);
    NS_ENSURE_TRUE(scope, false);

    mTabChildGlobal = scope;

    nsISupports* scopeSupports = NS_ISUPPORTS_CAST(nsIDOMEventTarget*, scope);

    NS_ENSURE_TRUE(InitTabChildGlobalInternal(scopeSupports), false);

    scope->Init();

    nsCOMPtr<nsPIWindowRoot> root = do_QueryInterface(chromeHandler);
    NS_ENSURE_TRUE(root,  false);
    root->SetParentTarget(scope);

    return true;
}

NS_IMETHODIMP
TabChildHelper::Observe(nsISupports *aSubject,
                        const char *aTopic,
                        const PRUnichar *aData)
{
    if (!strcmp(aTopic, CANCEL_DEFAULT_PAN_ZOOM)) {
        LOGNI("top:%s >>>>>>>>>>>>>.", aTopic);
        mView->SendCancelDefaultPanZoom();
    } else if (!strcmp(aTopic, BROWSER_ZOOM_TO_RECT)) {
        nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aSubject));
        gfxRect rect;
        sscanf(NS_ConvertUTF16toUTF8(aData).get(),
               "{\"x\":%lf,\"y\":%lf,\"w\":%lf,\"h\":%lf}",
               &rect.x, &rect.y, &rect.width, &rect.height);
        mView->SendZoomToRect(rect);
    } else if (!strcmp(aTopic, DETECT_SCROLLABLE_SUBFRAME)) {
        mView->SendDetectScrollableSubframe();
    }

    return NS_OK;
}

static void
ScrollWindowTo(nsIDOMWindow* aWindow, const mozilla::gfx::Point& aPoint)
{
    nsGlobalWindow* window = static_cast<nsGlobalWindow*>(aWindow);
    nsIScrollableFrame* sf = window->GetScrollFrame();

    if (sf) {
        sf->ScrollToCSSPixelsApproximate(aPoint);
    }
}

bool
TabChildHelper::RecvUpdateFrame(const FrameMetrics& aFrameMetrics)
{
    LOGF();
    gfx::Rect cssCompositedRect =
      AsyncPanZoomController::CalculateCompositedRectInCssPixels(aFrameMetrics);
    // The BrowserElementScrolling helper must know about these updated metrics
    // for other functions it performs, such as double tap handling.
//    mView->mScrolling->ViewportChange(aFrameMetrics, cssCompositedRect);

    nsString data;
    data.AppendPrintf("{ \"x\" : %d", NS_lround(aFrameMetrics.mScrollOffset.x));
    data.AppendPrintf(", \"y\" : %d", NS_lround(aFrameMetrics.mScrollOffset.y));
    data.AppendPrintf(", \"viewport\" : ");
        data.AppendPrintf("{ \"width\" : %f", aFrameMetrics.mViewport.width);
        data.AppendPrintf(", \"height\" : %f", aFrameMetrics.mViewport.height);
        data.AppendPrintf(" }");
    data.AppendPrintf(", \"displayPort\" : ");
        data.AppendPrintf("{ \"x\" : %f", aFrameMetrics.mDisplayPort.x);
        data.AppendPrintf(", \"y\" : %f", aFrameMetrics.mDisplayPort.y);
        data.AppendPrintf(", \"width\" : %f", aFrameMetrics.mDisplayPort.width);
        data.AppendPrintf(", \"height\" : %f", aFrameMetrics.mDisplayPort.height);
        data.AppendPrintf(" }");
    data.AppendPrintf(", \"compositionBounds\" : ");
        data.AppendPrintf("{ \"x\" : %d", aFrameMetrics.mCompositionBounds.x);
        data.AppendPrintf(", \"y\" : %d", aFrameMetrics.mCompositionBounds.y);
        data.AppendPrintf(", \"width\" : %d", aFrameMetrics.mCompositionBounds.width);
        data.AppendPrintf(", \"height\" : %d", aFrameMetrics.mCompositionBounds.height);
        data.AppendPrintf(" }");
    data.AppendPrintf(", \"cssPageRect\" : ");
        data.AppendPrintf("{ \"x\" : %f", aFrameMetrics.mScrollableRect.x);
        data.AppendPrintf(", \"y\" : %f", aFrameMetrics.mScrollableRect.y);
        data.AppendPrintf(", \"width\" : %f", aFrameMetrics.mScrollableRect.width);
        data.AppendPrintf(", \"height\" : %f", aFrameMetrics.mScrollableRect.height);
        data.AppendPrintf(" }");
    data.AppendPrintf(", \"cssCompositedRect\" : ");
            data.AppendPrintf("{ \"width\" : %f", cssCompositedRect.width);
            data.AppendPrintf(", \"height\" : %f", cssCompositedRect.height);
            data.AppendPrintf(" }");
    data.AppendPrintf(" }");

    RecvAsyncMessage(NS_LITERAL_STRING("Viewport:Change"), data);

    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mView->mWebNavigation);
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);

    utils->SetScrollPositionClampingScrollPortSize(
        cssCompositedRect.width, cssCompositedRect.height);
    ScrollWindowTo(window, aFrameMetrics.mScrollOffset);
    gfxSize resolution = AsyncPanZoomController::CalculateResolution(
        aFrameMetrics);
    utils->SetResolution(resolution.width, resolution.height);

    nsCOMPtr<nsIDOMDocument> domDoc;
    nsCOMPtr<nsIDOMElement> docElement;
    mView->mWebNavigation->GetDocument(getter_AddRefs(domDoc));
    if (domDoc) {
        domDoc->GetDocumentElement(getter_AddRefs(docElement));
        if (docElement) {
            utils->SetDisplayPortForElement(
                aFrameMetrics.mDisplayPort.x, aFrameMetrics.mDisplayPort.y,
                aFrameMetrics.mDisplayPort.width, aFrameMetrics.mDisplayPort.height,
                docElement);
        }
    }

    mLastMetrics = aFrameMetrics;

    return true;
}

nsIWebNavigation*
TabChildHelper::WebNavigation()
{
    return mView->mWebNavigation;
}

bool
TabChildHelper::DoLoadFrameScript(const nsAString& aURL)
{
    if (!mCx && !InitTabChildGlobal())
        // This can happen if we're half-destroyed.  It's not a fatal
        // error.
        return false;

    LoadFrameScriptInternal(aURL);
    return true;
}

static JSBool
JSONCreator(const jschar* aBuf, uint32_t aLen, void* aData)
{
    nsAString* result = static_cast<nsAString*>(aData);
    result->Append(static_cast<const PRUnichar*>(aBuf),
                   static_cast<uint32_t>(aLen));
    return true;
}

bool
TabChildHelper::DoSendSyncMessage(const nsAString& aMessage,
                                  const mozilla::dom::StructuredCloneData& aData,
                                  InfallibleTArray<nsString>* aJSONRetVal)
{
    if (!mView->HasMessageListener(aMessage)) {
        LOGE("Message not registered msg:%s\n", NS_ConvertUTF16toUTF8(aMessage).get());
        return false;
    }

    NS_ENSURE_TRUE(InitTabChildGlobal(), false);
    JSAutoRequest ar(mCx);

    // FIXME: Need callback interface for simple JSON to avoid useless conversion here
    jsval jv = JSVAL_NULL;
    if (aData.mDataLength &&
        !ReadStructuredClone(mCx, aData, &jv)) {
      JS_ClearPendingException(mCx);
      return false;
    }

    nsAutoString json;
    NS_ENSURE_TRUE(JS_Stringify(mCx, &jv, nullptr, JSVAL_NULL, JSONCreator, &json), false);
    NS_ENSURE_TRUE(!json.IsEmpty(), false);

    return mView->DoSendSyncMessage(nsString(aMessage).get(), json.get(), aJSONRetVal);
}

bool
TabChildHelper::DoSendAsyncMessage(const nsAString& aMessage,
                                   const mozilla::dom::StructuredCloneData& aData)
{
    if (!mView->HasMessageListener(aMessage)) {
        LOGE("Message not registered msg:%s\n", NS_ConvertUTF16toUTF8(aMessage).get());
        return false;
    }

    NS_ENSURE_TRUE(InitTabChildGlobal(), false);
    JSAutoRequest ar(mCx);

    // FIXME: Need callback interface for simple JSON to avoid useless conversion here
    jsval jv = JSVAL_NULL;
    if (aData.mDataLength &&
        !ReadStructuredClone(mCx, aData, &jv)) {
      JS_ClearPendingException(mCx);
      return false;
    }

    nsAutoString json;
    NS_ENSURE_TRUE(JS_Stringify(mCx, &jv, nullptr, JSVAL_NULL, JSONCreator, &json), false);
    NS_ENSURE_TRUE(!json.IsEmpty(), false);

    return mView->DoSendAsyncMessage(nsString(aMessage).get(), json.get());
}

bool
TabChildHelper::CheckPermission(const nsAString& aPermission)
{
    LOGNI("perm: %s", NS_ConvertUTF16toUTF8(aPermission).get());
    return false;
}

bool
TabChildHelper::RecvAsyncMessage(const nsAString& aMessageName,
                                 const nsAString& aJSONData)
{
    NS_ENSURE_TRUE(InitTabChildGlobal(), false);
    JSAutoRequest ar(mCx);
    jsval json = JSVAL_NULL;
    StructuredCloneData cloneData;
    JSAutoStructuredCloneBuffer buffer;
    if (JS_ParseJSON(mCx,
                      static_cast<const jschar*>(aJSONData.BeginReading()),
                      aJSONData.Length(),
                      &json)) {
        WriteStructuredClone(mCx, json, buffer, cloneData.mClosure);
        cloneData.mData = buffer.data();
        cloneData.mDataLength = buffer.nbytes();
    }

    nsFrameScriptCx cx(static_cast<nsIWebBrowserChrome*>(mView->mChrome), this);

    nsRefPtr<nsFrameMessageManager> mm =
      static_cast<nsFrameMessageManager*>(mTabChildGlobal->mMessageManager.get());
    mm->ReceiveMessage(static_cast<nsIDOMEventTarget*>(mTabChildGlobal),
                       aMessageName, false, &cloneData, nullptr, nullptr);
    return true;
}

static nsIntPoint
ToWidgetPoint(float aX, float aY, const nsPoint& aOffset,
              nsPresContext* aPresContext)
{
    double appPerDev = aPresContext->AppUnitsPerDevPixel();
    nscoord appPerCSS = nsPresContext::AppUnitsPerCSSPixel();
    return nsIntPoint(NSToIntRound((aX * appPerCSS + aOffset.x) / appPerDev),
                      NSToIntRound((aY * appPerCSS + aOffset.y) / appPerDev));
}

bool
TabChildHelper::ConvertMutiTouchInputToEvent(const mozilla::MultiTouchInput& aData,
                                             const gfxSize& res, const gfxPoint& diff,
                                             nsTouchEvent& aEvent)
{
    uint32_t msg = NS_USER_DEFINED_EVENT;
    switch (aData.mType) {
        case MultiTouchInput::MULTITOUCH_START: {
            msg = NS_TOUCH_START;
            break;
        }
        case MultiTouchInput::MULTITOUCH_MOVE: {
            msg = NS_TOUCH_MOVE;
            break;
        }
        case MultiTouchInput::MULTITOUCH_END: {
            msg = NS_TOUCH_END;
            break;
        }
        case MultiTouchInput::MULTITOUCH_ENTER: {
            msg = NS_TOUCH_ENTER;
            break;
        }
        case MultiTouchInput::MULTITOUCH_LEAVE: {
            msg = NS_TOUCH_LEAVE;
            break;
        }
        case MultiTouchInput::MULTITOUCH_CANCEL: {
            msg = NS_TOUCH_CANCEL;
            break;
        }
        default:
            return false;
    }
    // get the widget to send the event to
    nsPoint offset;
    nsCOMPtr<nsIWidget> widget = GetWidget(&offset);
    if (!widget)
        return false;

    aEvent.widget = widget;
    aEvent.mFlags.mIsTrusted = true;
    aEvent.message = msg;
    aEvent.eventStructType = NS_TOUCH_EVENT;
    aEvent.time = aData.mTime;

    nsPresContext* presContext = GetPresContext();
    if (!presContext) {
        return false;
    }

    for (uint32_t i = 0; i < aData.mTouches.Length(); ++i) {
        const SingleTouchData& data = aData.mTouches[i];
        gfx::Point pt(data.mScreenPoint.x, data.mScreenPoint.y);
        pt.x = pt.x / res.width;
        pt.y = pt.y / res.height;
        nsIntPoint tpt = ToWidgetPoint(pt.x, pt.y, offset, presContext);
        if (!getenv("TT1")) {
        tpt.x -= diff.x;
        tpt.y -= diff.y;
        }
        aEvent.touches.AppendElement(new nsDOMTouch(data.mIdentifier,
            tpt,
            data.mRadius,
            data.mRotationAngle,
            data.mForce));
    }

    return true;
}

nsIWidget*
TabChildHelper::GetWidget(nsPoint* aOffset)
{
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mView->mWebNavigation);
    NS_ENSURE_TRUE(window, nullptr);
    nsIDocShell *docShell = window->GetDocShell();
    NS_ENSURE_TRUE(docShell, nullptr);
    nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();
    NS_ENSURE_TRUE(presShell, nullptr);
    nsIFrame* frame = presShell->GetRootFrame();
    if (frame)
        return frame->GetView()->GetNearestWidget(aOffset);

    return nullptr;
}

nsPresContext*
TabChildHelper::GetPresContext()
{
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mView->mWebNavigation);
    NS_ENSURE_TRUE(window, nullptr);
    nsIDocShell *docShell = window->GetDocShell();
    NS_ENSURE_TRUE(docShell, nullptr);
    nsRefPtr<nsPresContext> presContext;
    docShell->GetPresContext(getter_AddRefs(presContext));
    return presContext;
}

void
TabChildHelper::InitEvent(nsGUIEvent& event, nsIntPoint* aPoint)
{
    if (aPoint) {
        event.refPoint.x = aPoint->x;
        event.refPoint.y = aPoint->y;
    } else {
        event.refPoint.x = 0;
        event.refPoint.y = 0;
    }

    event.time = PR_Now() / 1000;
}

nsEventStatus
TabChildHelper::DispatchWidgetEvent(nsGUIEvent& event)
{
    if (!mView->mWidget || !event.widget)
        return nsEventStatus_eConsumeNoDefault;

    event.mFlags.mIsBeingDispatched = false;

    nsEventStatus status;
    NS_ENSURE_SUCCESS(event.widget->DispatchEvent(&event, status),
                      nsEventStatus_eConsumeNoDefault);
    return status;
}

nsEventStatus
TabChildHelper::DispatchSynthesizedMouseEvent(const nsTouchEvent& aEvent)
{
    // Synthesize a phony mouse event.
    uint32_t msg;
    switch (aEvent.message) {
        case NS_TOUCH_START:
            msg = NS_MOUSE_BUTTON_DOWN;
            break;
        case NS_TOUCH_MOVE:
            msg = NS_MOUSE_MOVE;
            break;
        case NS_TOUCH_END:
        case NS_TOUCH_CANCEL:
            msg = NS_MOUSE_BUTTON_UP;
            break;
        default:
            MOZ_NOT_REACHED("Unknown touch event message");
    }

    // get the widget to send the event to
    nsPoint offset;
    nsCOMPtr<nsIWidget> widget = GetWidget(&offset);
    if (!widget)
        return nsEventStatus_eIgnore;

    nsMouseEvent event(true, msg, widget, nsMouseEvent::eReal, nsMouseEvent::eNormal);

    event.widget = widget;
    if (msg != NS_MOUSE_MOVE) {
        event.clickCount = 1;
    }
    event.time = PR_IntervalNow();

    nsPresContext* presContext = GetPresContext();
    if (!presContext)
        return nsEventStatus_eIgnore;

    nsIntPoint refPoint;
    if (aEvent.touches.Length()) {
        refPoint = aEvent.touches[0]->mRefPoint;
    }

    event.refPoint = ToWidgetPoint(refPoint.x, refPoint.y, offset, presContext);
    event.ignoreRootScrollFrame = true;

    nsEventStatus status;
    if NS_SUCCEEDED(widget->DispatchEvent(&event, status)) {
        return status;
    }
    return nsEventStatus_eIgnore;
}

void
TabChildHelper::DispatchSynthesizedMouseEvent(uint32_t aMsg, uint64_t aTime,
                                              const nsIntPoint& aRefPoint)
{
    // Synthesize a phony mouse event.
    MOZ_ASSERT(aMsg == NS_MOUSE_MOVE || aMsg == NS_MOUSE_BUTTON_DOWN ||
               aMsg == NS_MOUSE_BUTTON_UP);

    nsMouseEvent event(true, aMsg, NULL,
        nsMouseEvent::eReal, nsMouseEvent::eNormal);
    event.refPoint = aRefPoint;
    event.time = aTime;
    event.button = nsMouseEvent::eLeftButton;
    if (aMsg != NS_MOUSE_MOVE) {
        event.clickCount = 1;
    }

    DispatchWidgetEvent(event);
}
