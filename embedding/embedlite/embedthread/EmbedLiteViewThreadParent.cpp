/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteViewThreadParent"

#include "EmbedLiteViewThreadParent.h"
#include "EmbedLog.h"
#include "EmbedLiteApp.h"
#include "EmbedLiteView.h"

#include "EmbedLiteCompositorParent.h"
#include "mozilla/unused.h"
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/GeckoContentController.h"

using namespace mozilla::layers;

namespace mozilla {
namespace embedlite {

class EmbedContentController : public GeckoContentController {
public:
    EmbedContentController(EmbedLiteViewThreadParent* aRenderFrame)
      : mUILoop(MessageLoop::current())
      , mRenderFrame(aRenderFrame)
    { }

    virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) MOZ_OVERRIDE
    {
        // We always need to post requests into the "UI thread" otherwise the
        // requests may get processed out of order.
        LOGT();
        mUILoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &EmbedContentController::DoRequestContentRepaint,
                              aFrameMetrics));
    }

    virtual void HandleDoubleTap(const nsIntPoint& aPoint) MOZ_OVERRIDE
    {
        if (MessageLoop::current() != mUILoop) {
            // We have to send this message from the "UI thread" (main
            // thread).
            mUILoop->PostTask(
                FROM_HERE,
                NewRunnableMethod(this, &EmbedContentController::HandleDoubleTap,
                                  aPoint));
            return;
        }
        if (mRenderFrame) {
            unused << mRenderFrame->SendHandleDoubleTap(aPoint);
        }
    }

    virtual void HandleSingleTap(const nsIntPoint& aPoint) MOZ_OVERRIDE
    {
        if (MessageLoop::current() != mUILoop) {
            // We have to send this message from the "UI thread" (main
            // thread).
            mUILoop->PostTask(
                FROM_HERE,
                NewRunnableMethod(this, &EmbedContentController::HandleSingleTap,
                                  aPoint));
            return;
        }
        if (mRenderFrame) {
            unused << mRenderFrame->SendHandleSingleTap(aPoint);
        }
    }

    virtual void HandleLongTap(const nsIntPoint& aPoint) MOZ_OVERRIDE
    {
        if (MessageLoop::current() != mUILoop) {
            // We have to send this message from the "UI thread" (main
            // thread).
            mUILoop->PostTask(
                FROM_HERE,
                NewRunnableMethod(this, &EmbedContentController::HandleLongTap,
                                  aPoint));
            return;
        }
        if (mRenderFrame) {
            unused << mRenderFrame->SendHandleLongTap(aPoint);
        }
    }

    /**
     * Requests sending a mozbrowserasyncscroll domevent to embedder.
     * |aContentRect| is in CSS pixels, relative to the current cssPage.
     * |aScrollableSize| is the current content width/height in CSS pixels.
     */
    virtual void SendAsyncScrollDOMEvent(const gfx::Rect &aContentRect,
                                         const gfx::Size &aScrollableSize)
    {
        LOGNI();
    }

    void ClearRenderFrame() { mRenderFrame = nullptr; }

private:
    void DoRequestContentRepaint(const FrameMetrics& aFrameMetrics)
    {
        if (mRenderFrame) {
            unused << mRenderFrame->SendUpdateFrame(aFrameMetrics);
        }
    }

    MessageLoop* mUILoop;
    EmbedLiteViewThreadParent* mRenderFrame;
};

EmbedLiteViewThreadParent::EmbedLiteViewThreadParent(const uint32_t& id)
  : mId(id)
  , mView(EmbedLiteApp::GetInstance()->GetViewByID(id))
  , mCompositor(nullptr)
  , mScrollOffset(0, 0)
  , mLastScale(1.0f)
{
    MOZ_COUNT_CTOR(EmbedLiteViewThreadParent);
    LOGT("id:%u", mId);
    NS_ASSERTION(mView, "View not found");
    mView->SetImpl(this);
    mGeckoController = new EmbedContentController(this);
}

EmbedLiteViewThreadParent::~EmbedLiteViewThreadParent()
{
    MOZ_COUNT_DTOR(EmbedLiteViewThreadParent);
    LOGT();
    if (mGeckoController) {
        mGeckoController->ClearRenderFrame();
    }
    if (mView) {
        mView->SetImpl(NULL);
    }
}

void
EmbedLiteViewThreadParent::ActorDestroy(ActorDestroyReason aWhy)
{
    LOGT("reason:%i", aWhy);
    if (mGeckoController) {
        mGeckoController->ClearRenderFrame();
    }
}

void
EmbedLiteViewThreadParent::SetCompositor(EmbedLiteCompositorParent* aCompositor)
{
    LOGT();
    mCompositor = aCompositor;
    UpdateScrollController();
}

void
EmbedLiteViewThreadParent::UpdateScrollController()
{
    mController = nullptr;
    NS_ENSURE_TRUE(mView, );
    if (mView->GetPanZoomControlType() != EmbedLiteView::PanZoomControlType::EXTERNAL) {
        AsyncPanZoomController::GestureBehavior type;
        if (mView->GetPanZoomControlType() == EmbedLiteView::PanZoomControlType::GECKO_SIMPLE) {
            type = AsyncPanZoomController::DEFAULT_GESTURES;
        } else if (mView->GetPanZoomControlType() == EmbedLiteView::PanZoomControlType::GECKO_TOUCH) {
            type = AsyncPanZoomController::USE_GESTURE_DETECTOR;
        } else {
            return;
        }
        mController = new AsyncPanZoomController(mGeckoController, type);
        mController->SetCompositorParent(mCompositor);
    }
}

AsyncPanZoomController*
EmbedLiteViewThreadParent::GetDefaultPanZoomController()
{
    LOGT("t");
    return mController;
}

// Child notification

bool
EmbedLiteViewThreadParent::RecvInitialized()
{
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->ViewInitialized();
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnTitleChanged(const nsString& aTitle)
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnTitleChanged(aTitle.get());
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnLocationChanged(const nsCString& aLocation,
                                                 const bool& aCanGoBack,
                                                 const bool& aCanGoForward)
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnLocationChanged(aLocation.get(), aCanGoBack, aCanGoForward);
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnLoadStarted(const nsCString& aLocation)
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnLoadStarted(aLocation.get());
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnLoadFinished()
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnLoadFinished();
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnLoadRedirect()
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnLoadRedirect();
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnLoadProgress(const int32_t& aProgress, const int32_t& aCurTotal, const int32_t& aMaxTotal)
{
    LOGNI("progress:%i", aProgress);
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnLoadProgress(aProgress, aCurTotal, aMaxTotal);
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnSecurityChanged(const nsCString& aStatus,
                                                 const uint32_t& aState)
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnSecurityChanged(aStatus.get(), aState);
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnFirstPaint(const int32_t& aX,
                                            const int32_t& aY)
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnFirstPaint(aX, aY);
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnContentLoaded(const nsString& aDocURI)
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnContentLoaded(aDocURI.get());
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnLinkAdded(const nsString& aHref,
                                           const nsString& aCharset,
                                           const nsString& aTitle,
                                           const nsString& aRel,
                                           const nsString& aSizes,
                                           const nsString& aType)
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnLinkAdded(aHref.get(),
                                      aCharset.get(),
                                      aTitle.get(),
                                      aRel.get(),
                                      aSizes.get(),
                                      aType.get());
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnWindowOpenClose(const nsString& aType)
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnWindowOpenClose(aType.get());
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnPopupBlocked(const nsCString& aSpec,
                                              const nsCString& aCharset,
                                              const nsString& aPopupFeatures,
                                              const nsString& aPopupWinName)
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnPopupBlocked(aSpec.get(),
                                         aCharset.get(),
                                         aPopupFeatures.get(),
                                         aPopupWinName.get());
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnPageShowHide(const nsString& aType,
                                              const bool& aPersisted)
{
    LOGNI();
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnPageShowHide(aType.get(),
                                         aPersisted);
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnScrolledAreaChanged(const uint32_t& aWidth,
                                                     const uint32_t& aHeight)
{
    LOGNI("area[%u,%u]", aWidth, aHeight);
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnScrolledAreaChanged(aWidth, aHeight);
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnScrollChanged(const int32_t& offSetX,
                                               const int32_t& offSetY)
{
    LOGNI("off[%i,%i]", offSetX, offSetY);
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnScrollChanged(offSetX, offSetY);
    return true;
}

bool
EmbedLiteViewThreadParent::RecvOnObserve(const nsCString& aTopic,
                                         const nsString& aData)
{
    LOGNI("data:%p, top:%s\n", NS_ConvertUTF16toUTF8(aData).get(), aTopic.get());
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->OnObserve(aTopic.get(), aData.get());
    return true;
}

bool
EmbedLiteViewThreadParent::RecvUpdateZoomConstraints(const bool& val, const float& min, const float& max)
{
    if (mController) {
        mController->UpdateZoomConstraints(val, min, max);
    }
    return true;
}

bool
EmbedLiteViewThreadParent::RecvZoomToRect(const gfxRect& aRect)
{
    if (mController) {
        mController->ZoomToRect(aRect);
    }
    return true;
}

bool
EmbedLiteViewThreadParent::RecvCancelDefaultPanZoom()
{
    if (mController) {
        mController->CancelDefaultPanZoom();
    }
    return true;
}

bool
EmbedLiteViewThreadParent::RecvContentReceivedTouch(const bool& aPreventDefault)
{
    if (mController) {
        mController->ContentReceivedTouch(aPreventDefault);
    }
    return true;
}

bool
EmbedLiteViewThreadParent::RecvSetBackgroundColor(const nscolor& aColor)
{
    mView->GetListener()->SetBackgroundColor(NS_GET_R(aColor), NS_GET_G(aColor), NS_GET_B(aColor), NS_GET_A(aColor));
    return true;
}

// Incoming API calls

void
EmbedLiteViewThreadParent::LoadURL(const char* aUrl)
{
    LOGT("url:%s", aUrl);
    unused << SendLoadURL(NS_ConvertUTF8toUTF16(nsDependentCString(aUrl)));
}

void
EmbedLiteViewThreadParent::SetIsActive(bool aIsActive)
{
    LOGT();
    unused << SendSetIsActive(aIsActive);
}

void
EmbedLiteViewThreadParent::LoadFrameScript(const char* aURI)
{
    LOGT("uri:%s", aURI);
    unused << SendLoadFrameScript(NS_ConvertUTF8toUTF16(nsDependentCString(aURI)));
}

void
EmbedLiteViewThreadParent::DoSendAsyncMessage(const char* aMessageName, const char* aMessage)
{
    LOGT("msgName:%s, msg:%s", aMessageName, aMessage);
    unused << SendAsyncMessage(NS_ConvertUTF8toUTF16(nsDependentCString(aMessageName)),
                               NS_ConvertUTF8toUTF16(nsDependentCString(aMessage)));
}

bool
EmbedLiteViewThreadParent::RecvAsyncMessage(const nsString& aMessage,
                                            const nsString& aData)
{
    LOGT("msg:%s, data:%s", NS_ConvertUTF16toUTF8(aMessage).get(), NS_ConvertUTF16toUTF8(aData).get());
    NS_ENSURE_TRUE(mView, false);
    mView->GetListener()->RecvAsyncMessage(NS_ConvertUTF16toUTF8(aMessage).get(), NS_ConvertUTF16toUTF8(aData).get());
    return true;
}

bool
EmbedLiteViewThreadParent::RecvSyncMessage(const nsString& aMessage,
                                           const nsString& aJSON,
                                           InfallibleTArray<nsString>* aJSONRetVal)
{
    LOGT("msg:%s, data:%s", NS_ConvertUTF16toUTF8(aMessage).get(), NS_ConvertUTF16toUTF8(aJSON).get());
    NS_ENSURE_TRUE(mView, false);
    const char* retval =
        mView->GetListener()->
            RecvSyncMessage(NS_ConvertUTF16toUTF8(aMessage).get(),
                            NS_ConvertUTF16toUTF8(aJSON).get());
    if (retval) {
        aJSONRetVal->AppendElement(NS_ConvertUTF8toUTF16(nsDependentCString(retval)));
    }
    return true;
}

static inline gfxASurface::gfxImageFormat
_depth_to_gfxformat(int depth)
{
    switch (depth) {
    case 32:
        return gfxASurface::ImageFormatARGB32;
    case 24:
        return gfxASurface::ImageFormatRGB24;
    case 16:
        return gfxASurface::ImageFormatRGB16_565;
    default:
        return gfxASurface::ImageFormatUnknown;
    }
}

void
EmbedLiteViewThreadParent::RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth)
{
    LOGT();
    if (mCompositor) {
        nsRefPtr<gfxImageSurface> source =
            new gfxImageSurface(aData, gfxIntSize(imgW, imgH), stride, _depth_to_gfxformat(depth));
        nsRefPtr<gfxContext> context = new gfxContext(source);
        mCompositor->RenderToContext(context);
    }
}

void
EmbedLiteViewThreadParent::RenderGL()
{
    LOGT();
    if (mCompositor) {
        mCompositor->RenderGL();
    }
}

bool
EmbedLiteViewThreadParent::ScrollBy(int aDX, int aDY, bool aDoOverflow)
{
    LOGT("d[%i,%i]", aDX, aDY);
    mScrollOffset.MoveBy(-aDX, -aDY);
    if (mCompositor) {
        mCompositor->SetTransformation(mLastScale, nsIntPoint(mScrollOffset.x, mScrollOffset.y));
        mCompositor->ScheduleRenderOnCompositorThread();
    }

    return true;
}

void
EmbedLiteViewThreadParent::SetViewSize(int width, int height)
{
    LOGT("sz[%i,%i]", width, height);
    mViewSize = gfxSize(width, height);
    unused << SendSetViewSize(mViewSize);
    if (mController) {
        mController->UpdateCompositionBounds(nsIntRect(0, 0, width, height));
    }
}

void
EmbedLiteViewThreadParent::SetGLViewPortSize(int width, int height)
{
    if (mCompositor) {
        mCompositor->SetSurfaceSize(width, height);
    }
}

void
EmbedLiteViewThreadParent::SetGLViewTransform(gfxMatrix matrix)
{
    if (mCompositor) {
        mCompositor->SetWorldTransform(matrix);
        mCompositor->SetClipping(gfxRect(gfxPoint(0, 0), mViewSize));
    }
}

void
EmbedLiteViewThreadParent::SetTransformation(float aScale, nsIntPoint aScrollOffset)
{
    if (mCompositor) {
        mCompositor->SetTransformation(aScale, aScrollOffset);
    }
}

void
EmbedLiteViewThreadParent::ScheduleRender()
{
    if (mCompositor) {
        mCompositor->ScheduleRenderOnCompositorThread();
    }
}

static gfx::Point
WidgetSpaceToCompensatedViewportSpace(const gfx::Point& aPoint,
                                      gfxFloat aCurrentZoom)
{
    // Transform the input point from local widget space to the content document
    // space that the user is seeing, from last composite.
    gfx::Point pt(aPoint);
    pt = pt / aCurrentZoom;

    // FIXME/bug 775451: this doesn't attempt to compensate for content transforms
    // in effect on the compositor.  The problem is that it's very hard for us to
    // know what content CSS pixel is at widget point 0,0 based on information
    // available here.  So we use this hacky implementation for now, which works
    // in quiescent states.

    return pt;
}

void
EmbedLiteViewThreadParent::ReceiveInputEvent(const InputData& aEvent)
{
    if (mController) {
        nsEventStatus status;
        status = mController->ReceiveInputEvent(aEvent);
        if (aEvent.mInputType == MULTITOUCH_INPUT) {
            const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();
            const SingleTouchData& data = multiTouchInput.mTouches[0];
            gfxSize sz = mController->CalculateResolution();
            if (multiTouchInput.mType == MultiTouchInput::MULTITOUCH_MOVE)
                unused << SendInputDataTouchMoveEvent(multiTouchInput, sz);
            else
                unused << SendInputDataTouchEvent(multiTouchInput, sz);
        }
    }
}

void
EmbedLiteViewThreadParent::TextEvent(const char* composite, const char* preEdit)
{
    LOGT("commit:%s, pre:%s", composite, preEdit);
    unused << SendHandleTextEvent(NS_ConvertUTF8toUTF16(nsDependentCString(composite)),
                                  NS_ConvertUTF8toUTF16(nsDependentCString(preEdit)));
}

void
EmbedLiteViewThreadParent::SendKeyPress(int domKeyCode, int gmodifiers, int charCode)
{
    LOGT("dom:%i, mod:%i, char:'%c'", domKeyCode, gmodifiers, charCode);
    unused << SendHandleKeyPressEvent(domKeyCode, gmodifiers, charCode);
}

void
EmbedLiteViewThreadParent::SendKeyRelease(int domKeyCode, int gmodifiers, int charCode)
{
    LOGT("dom:%i, mod:%i, char:'%c'", domKeyCode, gmodifiers, charCode);
    unused << SendHandleKeyReleaseEvent(domKeyCode, gmodifiers, charCode);
}

void
EmbedLiteViewThreadParent::MousePress(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    LOGT("pt[%i,%i], t:%i, bt:%u, mod:%u", x, y, mstime, buttons, modifiers);
    if (mController) {
        nsEventStatus status;
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_START, mstime);
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(x, y),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        status = mController->ReceiveInputEvent(event);
        unused << SendMouseEvent(NS_LITERAL_STRING("mousedown"),
                                 x, y, buttons, 1, modifiers,
                                 true);
    }
}

void
EmbedLiteViewThreadParent::MouseRelease(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    LOGT("pt[%i,%i], t:%i, bt:%u, mod:%u", x, y, mstime, buttons, modifiers);
    if (mController) {
        nsEventStatus status;
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_END, mstime);
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(x, y),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        status = mController->ReceiveInputEvent(event);
        unused << SendMouseEvent(NS_LITERAL_STRING("mouseup"),
                                 x, y, buttons, 1, modifiers,
                                 true);
    }
}

void
EmbedLiteViewThreadParent::MouseMove(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    LOGT("pt[%i,%i], t:%i, bt:%u, mod:%u", x, y, mstime, buttons, modifiers);
    if (mController) {
        nsEventStatus status;
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_MOVE, mstime);
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(x, y),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        status = mController->ReceiveInputEvent(event);
        unused << SendMouseEvent(NS_LITERAL_STRING("mousemove"),
                                 x, y, buttons, 1, modifiers,
                                 true);
    }
}

} // namespace embedlite
} // namespace mozilla
