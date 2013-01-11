/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_VIEW_EMBED_THREAD_PARENT_H
#define MOZ_VIEW_EMBED_THREAD_PARENT_H

#include "mozilla/embedlite/PEmbedLiteViewParent.h"
#include "EmbedLiteViewImplIface.h"

namespace mozilla {
namespace layers {
class AsyncPanZoomController;
}
namespace embedlite {

class EmbedLiteView;
class EmbedLiteCompositorParent;
class EmbedContentController;
class EmbedLiteViewThreadParent : public PEmbedLiteViewParent,
                                  public EmbedLiteViewImplIface
{
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(EmbedLiteViewThreadParent)
public:
    EmbedLiteViewThreadParent(const uint32_t& id);
    virtual ~EmbedLiteViewThreadParent();

    virtual void LoadURL(const char*);
    virtual void SetIsActive(bool);
    virtual void LoadFrameScript(const char* aURI);
    virtual void DoSendAsyncMessage(const char* aMessageName, const char* aMessage);
    virtual void RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth);
    virtual void RenderGL();
    virtual void SetViewSize(int width, int height);
    virtual void SetGLViewPortSize(int width, int height);
    virtual void SetGLViewTransform(gfxMatrix matrix);
    virtual void SetTransformation(float aScale, nsIntPoint aScrollOffset);
    virtual void ScheduleRender();
    virtual void UpdateScrollController();
    virtual bool ScrollBy(int aDX, int aDY, bool aDoOverflow = false);
    virtual void MousePress(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers);
    virtual void MouseRelease(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers);
    virtual void MouseMove(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers);
    virtual void ReceiveInputEvent(const InputData& aEvent);
    virtual void TextEvent(const char* composite, const char* preEdit);
    virtual void SendKeyPress(int,int,int);
    virtual void SendKeyRelease(int,int,int);
    virtual void ViewAPIDestroyed() { mView = nullptr; }
    mozilla::layers::AsyncPanZoomController* GetDefaultPanZoomController();

protected:
    virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
    virtual bool RecvInitialized();

    virtual bool
    RecvOnTitleChanged(const nsString& aTitle);

    virtual bool
    RecvOnLocationChanged(const nsCString& aLocation, const bool& aCanGoBack, const bool& aCanGoForward);

    virtual bool
    RecvOnLoadStarted(const nsCString& aLocation);

    virtual bool
    RecvOnLoadFinished();

    virtual bool
    RecvOnLoadRedirect();

    virtual bool
    RecvOnLoadProgress(const int32_t& aProgress, const int32_t& aCurTotal, const int32_t& aMaxTotal);

    virtual bool
    RecvOnSecurityChanged(
            const nsCString& aStatus,
            const uint32_t& aState);

    virtual bool
    RecvOnFirstPaint(
            const int32_t& aX,
            const int32_t& aY);

    virtual bool
    RecvOnContentLoaded(const nsString& aDocURI);

    virtual bool
    RecvOnLinkAdded(
            const nsString& aHref,
            const nsString& aCharset,
            const nsString& aTitle,
            const nsString& aRel,
            const nsString& aSizes,
            const nsString& aType);

    virtual bool
    RecvOnWindowOpenClose(const nsString& aType);

    virtual bool
    RecvOnPopupBlocked(
            const nsCString& aSpec,
            const nsCString& aCharset,
            const nsString& aPopupFeatures,
            const nsString& aPopupWinName);

    virtual bool
    RecvOnPageShowHide(
            const nsString& aType,
            const bool& aPersisted);

    virtual bool
    RecvOnScrolledAreaChanged(
            const uint32_t& aWidth,
            const uint32_t& aHeight);

    virtual bool
    RecvOnScrollChanged(
            const int32_t& offSetX,
            const int32_t& offSetY);

    virtual bool
    RecvOnObserve(
            const nsCString& aTopic,
            const nsString& aData);
    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const nsString& aData);
    virtual bool RecvSyncMessage(const nsString& aMessage,
                                 const nsString& aJSON,
                                 InfallibleTArray<nsString>* aJSONRetVal);
    virtual bool
    RecvUpdateZoomConstraints(const bool&, const float&, const float&);
    virtual bool RecvZoomToRect(const gfxRect& aRect);
    virtual bool RecvSetBackgroundColor(const nscolor& aColor);
    virtual bool RecvCancelDefaultPanZoom() MOZ_OVERRIDE;
    virtual bool RecvContentReceivedTouch(const bool& aPreventDefault);
    virtual bool RecvDetectScrollableSubframe() MOZ_OVERRIDE;

private:
    friend class EmbedLiteCompositorParent;
    void SetCompositor(EmbedLiteCompositorParent* aCompositor);
    uint32_t mId;
    EmbedLiteView* mView;
    RefPtr<EmbedLiteCompositorParent> mCompositor;
    gfx::Point mScrollOffset;
    float mLastScale;

    RefPtr<mozilla::layers::AsyncPanZoomController> mController;
    RefPtr<EmbedContentController> mGeckoController;

    gfxSize mViewSize;
    bool mInTouchProcess;

    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteViewThreadParent);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_VIEW_EMBED_THREAD_PARENT_H
