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
    virtual void GoBack();
    virtual void GoForward();
    virtual void StopLoad();
    virtual void Reload(bool hardReload);
    virtual void SetIsActive(bool);
    virtual void LoadFrameScript(const char* aURI);
    virtual void DoSendAsyncMessage(const char* aMessageName, const char* aMessage);
    virtual bool RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth);
    virtual bool RenderGL();
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
    virtual void ViewAPIDestroyed();
    mozilla::layers::AsyncPanZoomController* GetDefaultPanZoomController();
    virtual void UnblockPrompt(uint64_t winid, const bool&, const bool&, const nsString&, const nsString&, const nsString&);

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

    // prompt API
    virtual bool RecvAlert(const nsString&, const nsString&,
                           const nsString& checkMessage,
                           const bool& checkValue, const uint64_t& winID);
    virtual bool RecvConfirm(const nsString&, const nsString&,
                             const nsString& checkMessage,
                             const bool& checkValue, const uint64_t& winID);

    virtual bool RecvPrompt(const nsString&, const nsString&,
                             const nsString& checkMessage, const nsString& defaultValue,
                             const bool& checkValue, const uint64_t& winID);
    virtual bool
    RecvAuthentificationRequired(const uint64_t& requestID,
            const nsCString& hostname,
            const nsCString& httprealm,
            const nsString& username,
            const bool& isOnlyPassword);

    // IME
    virtual bool RecvGetInputContext(int32_t* aIMEEnabled,
                                     int32_t* aIMEOpen,
                                     intptr_t* aNativeIMEContext);
    virtual bool RecvSetInputContext(const int32_t& aIMEEnabled,
                                     const int32_t& aIMEOpen,
                                     const nsString& aType,
                                     const nsString& aInputmode,
                                     const nsString& aActionHint,
                                     const int32_t& aCause,
                                     const int32_t& aFocusChange);

private:
    friend class EmbedLiteCompositorParent;
    void SetCompositor(EmbedLiteCompositorParent* aCompositor);
    uint32_t mId;
    EmbedLiteView* mView;
    bool mViewAPIDestroyed;
    RefPtr<EmbedLiteCompositorParent> mCompositor;
    gfx::Point mScrollOffset;
    float mLastScale;

    RefPtr<mozilla::layers::AsyncPanZoomController> mController;
    RefPtr<EmbedContentController> mGeckoController;

    gfxSize mViewSize;
    bool mInTouchProcess;
    MessageLoop* mUILoop;
    int mLastIMEState;

    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteViewThreadParent);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_VIEW_EMBED_THREAD_PARENT_H
