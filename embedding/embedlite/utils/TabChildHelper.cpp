/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "TabChildHelper"

#include "TabChildHelper.h"
#include "EmbedLog.h"

#include "EmbedLiteViewThreadChild.h"
#include "EmbedLiteViewScrolling.h"
#include "mozilla/layers/AsyncPanZoomController.h"

#include "nsIObserverService.h"
#include "nsNetUtil.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMWindowUtils.h"
#include "nsIDOMElement.h"
#include "mozilla/dom/Element.h"
#include "nsGlobalWindow.h"
#include "nsIDocShell.h"
#include "nsViewportInfo.h"
#include "nsPIWindowRoot.h"
#include "StructuredCloneUtils.h"
#include "mozilla/Preferences.h"

static const nsIntSize kDefaultViewportSize(980, 480);

static const char CANCEL_DEFAULT_PAN_ZOOM[] = "cancel-default-pan-zoom";
static const char BROWSER_ZOOM_TO_RECT[] = "browser-zoom-to-rect";
static const char BEFORE_FIRST_PAINT[] = "before-first-paint";
static bool HANDLE_HTML_VIEWPORT = true;

using namespace mozilla::embedlite;
using namespace mozilla::layers;
using namespace mozilla::dom;

static bool GetHandleViewport()
{
    static bool sHandleViewport;
    static bool sHandleViewportCached = false;
    if (!sHandleViewportCached) {
        mozilla::Preferences::AddBoolVarCache(&sHandleViewport, "embedlite.handle_viewport", true);
        sHandleViewportCached = true;
    }
    return sHandleViewport;
}

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
                                     BEFORE_FIRST_PAINT,
                                     false);
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
  observerService->RemoveObserver(this, BEFORE_FIRST_PAINT);
}

NS_IMPL_ISUPPORTS1(TabChildHelper, nsIObserver)

bool
TabChildHelper::InitTabChildGlobal()
{
    if (!mCx && !mTabChildGlobal) {
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
    }

    return true;
}

NS_IMETHODIMP
TabChildHelper::Observe(nsISupports *aSubject,
                        const char *aTopic,
                        const PRUnichar *aData)
{
    if (!strcmp(aTopic, CANCEL_DEFAULT_PAN_ZOOM)) {
        LOGNI("top:%s >>>>>>>>>>>>>.", aTopic);
    } else if (!strcmp(aTopic, BROWSER_ZOOM_TO_RECT)) {
        nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aSubject));
        LOGNI("top:%s >>>>>>>>>>>>>.", aTopic);
        gfxRect rect;
        mView->SendZoomToRect(rect);
    } else if (!strcmp(aTopic, BEFORE_FIRST_PAINT) && GetHandleViewport()) {
        nsCOMPtr<nsIDocument> subject(do_QueryInterface(aSubject));
        nsCOMPtr<nsIDOMDocument> domDoc;
        mView->mWebNavigation->GetDocument(getter_AddRefs(domDoc));
        nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));

        if (SameCOMIdentity(subject, doc)) {
            nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mView->mWebNavigation);
            nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);

            mContentDocumentIsDisplayed = true;

            // Reset CSS viewport and zoom to default on new page, then
            // calculate them properly using the actual metadata from the
            // page.
            SetCSSViewport(kDefaultViewportSize.width, kDefaultViewportSize.height);

            // Calculate a really simple resolution that we probably won't
            // be keeping, as well as putting the scroll offset back to
            // the top-left of the page.
            mLastMetrics.mZoom = gfxSize(1.0, 1.0);
            mLastMetrics.mViewport =
                gfx::Rect(0, 0,
                          kDefaultViewportSize.width, kDefaultViewportSize.height);
            mLastMetrics.mCompositionBounds = nsIntRect(nsIntPoint(0, 0),
                                                        mInnerSize);
            mLastMetrics.mResolution =
                AsyncPanZoomController::CalculateResolution(mLastMetrics);
            mLastMetrics.mScrollOffset = gfx::Point(0, 0);
            utils->SetResolution(mLastMetrics.mResolution.width,
                                 mLastMetrics.mResolution.height);

            HandlePossibleViewportChange();
        }
    }

    return NS_OK;
}

void
TabChildHelper::SetCSSViewport(float aWidth, float aHeight)
{
  mOldViewportWidth = aWidth;

  if (mContentDocumentIsDisplayed) {
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mView->mWebNavigation);
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);

    utils->SetCSSViewport(aWidth, aHeight);
  }
}

void
TabChildHelper::HandlePossibleViewportChange()
{
  if (!GetHandleViewport()) {
    return;
  }

  nsCOMPtr<nsIDOMDocument> domDoc;
  mView->mWebNavigation->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDoc));

  nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mView->mWebNavigation);
  nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);

  nsViewportInfo viewportInfo =
    nsContentUtils::GetViewportInfo(document, mInnerSize.width, mInnerSize.height);
  mView->SendUpdateZoomConstraints(viewportInfo.IsZoomAllowed(),
                                   viewportInfo.GetMinZoom(),
                                   viewportInfo.GetMaxZoom());

  float screenW = mInnerSize.width;
  float screenH = mInnerSize.height;
  float viewportW = viewportInfo.GetWidth();
  float viewportH = viewportInfo.GetHeight();

  // We're not being displayed in any way; don't bother doing anything because
  // that will just confuse future adjustments.
  if (!screenW || !screenH) {
    return;
  }

  // Make sure the viewport height is not shorter than the window when the page
  // is zoomed out to show its full width. Note that before we set the viewport
  // width, the "full width" of the page isn't properly defined, so that's why
  // we have to call SetCSSViewport twice - once to set the width, and the
  // second time to figure out the height based on the layout at that width.
  float oldBrowserWidth = mOldViewportWidth;
  mLastMetrics.mViewport.width = viewportW;
  mLastMetrics.mViewport.height = viewportH;
  if (!oldBrowserWidth) {
    oldBrowserWidth = kDefaultViewportSize.width;
  }
  SetCSSViewport(viewportW, viewportH);

  // If this page has not been painted yet, then this must be getting run
  // because a meta-viewport element was added (via the DOMMetaAdded handler).
  // in this case, we should not do anything that forces a reflow (see bug
  // 759678) such as requesting the page size or sending a viewport update. this
  // code will get run again in the before-first-paint handler and that point we
  // will run though all of it. the reason we even bother executing up to this
  // point on the DOMMetaAdded handler is so that scripts that use
  // window.innerWidth before they are painted have a correct value (bug
  // 771575).
  if (!mContentDocumentIsDisplayed) {
    return;
  }

  float minScale = 1.0f;

  nsCOMPtr<nsIDOMElement> htmlDOMElement = do_QueryInterface(document->GetHtmlElement());
  nsCOMPtr<nsIDOMElement> bodyDOMElement = do_QueryInterface(document->GetBodyElement());

  int32_t htmlWidth = 0, htmlHeight = 0;
  if (htmlDOMElement) {
    htmlDOMElement->GetScrollWidth(&htmlWidth);
    htmlDOMElement->GetScrollHeight(&htmlHeight);
  }
  int32_t bodyWidth = 0, bodyHeight = 0;
  if (bodyDOMElement) {
    bodyDOMElement->GetScrollWidth(&bodyWidth);
    bodyDOMElement->GetScrollHeight(&bodyHeight);
  }

  float pageWidth, pageHeight;
  if (htmlDOMElement || bodyDOMElement) {
    pageWidth = NS_MAX(htmlWidth, bodyWidth);
    pageHeight = NS_MAX(htmlHeight, bodyHeight);
  } else {
    // For non-HTML content (e.g. SVG), just assume page size == viewport size.
    pageWidth = viewportW;
    pageHeight = viewportH;
  }
  NS_ENSURE_TRUE_VOID(pageWidth); // (return early rather than divide by 0)

  minScale = mInnerSize.width / pageWidth;
  minScale = clamped((double)minScale, viewportInfo.GetMinZoom(),
                     viewportInfo.GetMaxZoom());
  NS_ENSURE_TRUE_VOID(minScale); // (return early rather than divide by 0)

  viewportH = NS_MAX(viewportH, screenH / minScale);
  SetCSSViewport(viewportW, viewportH);

  // This change to the zoom accounts for all types of changes I can conceive:
  // 1. screen size changes, CSS viewport does not (pages with no meta viewport
  //    or a fixed size viewport)
  // 2. screen size changes, CSS viewport also does (pages with a device-width
  //    viewport)
  // 3. screen size remains constant, but CSS viewport changes (meta viewport
  //    tag is added or removed)
  // 4. neither screen size nor CSS viewport changes
  //
  // In all of these cases, we maintain how much actual content is visible
  // within the screen width. Note that "actual content" may be different with
  // respect to CSS pixels because of the CSS viewport size changing.
  int32_t oldScreenWidth = mLastMetrics.mCompositionBounds.width;
  if (!oldScreenWidth) {
    oldScreenWidth = mInnerSize.width;
  }

  FrameMetrics metrics(mLastMetrics);
  metrics.mViewport = gfx::Rect(0.0f, 0.0f, viewportW, viewportH);
  metrics.mScrollableRect = gfx::Rect(0.0f, 0.0f, pageWidth, pageHeight);
  metrics.mCompositionBounds = nsIntRect(0, 0, mInnerSize.width, mInnerSize.height);

  // Changing the zoom when we're not doing a first paint will get ignored
  // by AsyncPanZoomController and causes a blurry flash.
  bool isFirstPaint;
  nsresult rv = utils->GetIsFirstPaint(&isFirstPaint);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  if (NS_FAILED(rv) || isFirstPaint) {
    gfxSize intrinsicScale =
        AsyncPanZoomController::CalculateIntrinsicScale(metrics);
    // FIXME/bug 799585(?): GetViewportInfo() returns a defaultZoom of
    // 0.0 to mean "did not calculate a zoom".  In that case, we default
    // it to the intrinsic scale.
    if (viewportInfo.GetDefaultZoom() < 0.01f) {
      viewportInfo.SetDefaultZoom(intrinsicScale.width);
    }

    double defaultZoom = viewportInfo.GetDefaultZoom();
    MOZ_ASSERT(viewportInfo.GetMinZoom() <= defaultZoom &&
               defaultZoom <= viewportInfo.GetMaxZoom());
    // GetViewportInfo() returns a resolution-dependent scale factor.
    // Convert that to a resolution-indepedent zoom.
    metrics.mZoom = gfxSize(defaultZoom / intrinsicScale.width,
                            defaultZoom / intrinsicScale.height);
  }

  metrics.mDisplayPort = AsyncPanZoomController::CalculatePendingDisplayPort(
    // The page must have been refreshed in some way such as a new document or
    // new CSS viewport, so we know that there's no velocity, acceleration, and
    // we have no idea how long painting will take.
    metrics, gfx::Point(0.0f, 0.0f), gfx::Point(0.0f, 0.0f), 0.0);
  gfxSize resolution = AsyncPanZoomController::CalculateResolution(metrics);
  // XXX is this actually hysteresis?  This calculation is not well
  // understood.  It's taken from the previous JS implementation.
  gfxFloat hysteresis/*?*/ =
    gfxFloat(oldBrowserWidth) / gfxFloat(oldScreenWidth);
  resolution.width *= hysteresis;
  resolution.height *= hysteresis;
  metrics.mResolution = resolution;
  utils->SetResolution(metrics.mResolution.width, metrics.mResolution.height);

  // Force a repaint with these metrics. This, among other things, sets the
  // displayport, so we start with async painting.
  RecvUpdateFrame(metrics);
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
    gfx::Rect cssCompositedRect =
      AsyncPanZoomController::CalculateCompositedRectInCssPixels(aFrameMetrics);
    // The BrowserElementScrolling helper must know about these updated metrics
    // for other functions it performs, such as double tap handling.
    mView->mScrolling->ViewportChange(aFrameMetrics, cssCompositedRect);

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

    return mView->SendSyncMessage(nsString(aMessage), json, aJSONRetVal);
}

bool
TabChildHelper::DoSendAsyncMessage(const nsAString& aMessage,
                                   const mozilla::dom::StructuredCloneData& aData)
{
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

    mView->SendAsyncMessage(nsString(aMessage), json);

    return true;
}

bool
TabChildHelper::CheckPermission(const nsAString& aPermission)
{
    LOGNI("perm: %s", NS_ConvertUTF16toUTF8(aPermission).get());
    return false;
}

bool
TabChildHelper::RecvAsyncMessage(const nsString& aMessageName,
                                 const nsString& aJSONData)
{
    JSAutoRequest ar(mCx);
    jsval json = JSVAL_NULL;
    StructuredCloneData cloneData;
    JSAutoStructuredCloneBuffer buffer;
    if (JS_ParseJSON(mCx,
                      static_cast<const jschar*>(aJSONData.get()),
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

EmbedTabChildGlobal::EmbedTabChildGlobal(TabChildHelper* aTabChild)
  : mTabChild(aTabChild)
{
}

void
EmbedTabChildGlobal::Init()
{
  NS_ASSERTION(!mMessageManager, "Re-initializing?!?");
  mMessageManager = new nsFrameMessageManager(mTabChild,
                                              nullptr,
                                              mTabChild->GetJSContext(),
                                              mozilla::dom::ipc::MM_CHILD);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(EmbedTabChildGlobal)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(EmbedTabChildGlobal,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mMessageManager)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(EmbedTabChildGlobal,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMessageManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(EmbedTabChildGlobal)
  NS_INTERFACE_MAP_ENTRY(nsIMessageListenerManager)
  NS_INTERFACE_MAP_ENTRY(nsIMessageSender)
  NS_INTERFACE_MAP_ENTRY(nsISyncMessageSender)
  NS_INTERFACE_MAP_ENTRY(nsIContentFrameMessageManager)
  NS_INTERFACE_MAP_ENTRY(nsIScriptContextPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsITabChild)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ContentFrameMessageManager)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(EmbedTabChildGlobal, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(EmbedTabChildGlobal, nsDOMEventTargetHelper)

/* [notxpcom] boolean markForCC (); */
// This method isn't automatically forwarded safely because it's notxpcom, so
// the IDL binding doesn't know what value to return.
NS_IMETHODIMP_(bool)
EmbedTabChildGlobal::MarkForCC()
{
    return mMessageManager ? mMessageManager->MarkForCC() : false;
}

NS_IMETHODIMP
EmbedTabChildGlobal::GetContent(nsIDOMWindow** aContent)
{
    *aContent = nullptr;
    if (!mTabChild)
        return NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIDOMWindow> window = do_GetInterface(mTabChild->WebNavigation());
    window.swap(*aContent);
    return NS_OK;
}

NS_IMETHODIMP
EmbedTabChildGlobal::PrivateNoteIntentionalCrash()
{
//    mozilla::NoteIntentionalCrash("tab");
    return NS_OK;
}

NS_IMETHODIMP
EmbedTabChildGlobal::GetDocShell(nsIDocShell** aDocShell)
{
    *aDocShell = nullptr;
    if (!mTabChild)
        return NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mTabChild->WebNavigation());
    docShell.swap(*aDocShell);
    return NS_OK;
}

NS_IMETHODIMP
EmbedTabChildGlobal::Btoa(const nsAString& aBinaryData,
                          nsAString& aAsciiBase64String)
{
  return nsContentUtils::Btoa(aBinaryData, aAsciiBase64String);
}

NS_IMETHODIMP
EmbedTabChildGlobal::Atob(const nsAString& aAsciiString,
                          nsAString& aBinaryData)
{
  return nsContentUtils::Atob(aAsciiString, aBinaryData);
}

JSContext*
EmbedTabChildGlobal::GetJSContextForEventHandlers()
{
    if (!mTabChild)
        return nullptr;
    return mTabChild->GetJSContext();
}

nsIPrincipal*
EmbedTabChildGlobal::GetPrincipal()
{
    if (!mTabChild)
        return nullptr;
    return mTabChild->GetPrincipal();
}

NS_IMETHODIMP
EmbedTabChildGlobal::GetMessageManager(nsIContentFrameMessageManager** aResult)
{
    NS_ADDREF(*aResult = this);
    return NS_OK;
}
