/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_VIEW_EMBED_THREAD_CHILD_H
#define MOZ_VIEW_EMBED_THREAD_CHILD_H

#include "mozilla/embedlite/PEmbedLiteViewChild.h"

#include "nsIWebBrowser.h"
#include "nsIWidget.h"
#include "nsIWebNavigation.h"
#include "WebBrowserChrome.h"
#include "nsIEmbedBrowserChromeListener.h"
#include "TabChildHelper.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteViewScrolling;
class EmbedLitePuppetWidget;
class EmbedLiteAppThreadChild;

class EmbedLiteViewThreadChild : public PEmbedLiteViewChild,
                                 public nsIEmbedBrowserChromeListener
{
    NS_INLINE_DECL_REFCOUNTING(EmbedLiteViewThreadChild)
public:
    EmbedLiteViewThreadChild(uint32_t);
    virtual ~EmbedLiteViewThreadChild();

    NS_DECL_NSIEMBEDBROWSERCHROMELISTENER

    uint64_t GetOuterID() { return mOuterId; }
    void ResetInputState();

    void RecvAsyncMessage(const nsAString& aMessage,
                          const nsAString& aData);

    JSContext* GetJSContext() { return mHelper ? mHelper->GetJSContext() : nullptr; }

protected:
    virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
    virtual bool RecvDestroy();
    virtual bool RecvLoadURL(const nsString&);
    virtual bool RecvGoBack();
    virtual bool RecvGoForward();
    virtual bool RecvStopLoad();
    virtual bool RecvReload(const bool&);

    virtual bool RecvSetIsActive(const bool&);
    virtual bool RecvSuspendTimeouts();
    virtual bool RecvResumeTimeouts();
    virtual bool RecvLoadFrameScript(const nsString&);
    virtual bool RecvSetViewSize(const gfxSize&);
    virtual bool RecvAsyncScrollDOMEvent(
            const gfxRect& contentRect,
            const gfxSize& scrollSize);

    virtual bool RecvUpdateFrame(const mozilla::layers::FrameMetrics& aFrameMetrics);
    virtual bool RecvHandleDoubleTap(const nsIntPoint& aPoint);
    virtual bool RecvHandleSingleTap(const nsIntPoint& aPoint);
    virtual bool RecvHandleLongTap(const nsIntPoint& aPoint);
    virtual bool RecvMouseEvent(const nsString& aType,
                         const float&    aX,
                         const float&    aY,
                         const int32_t&  aButton,
                         const int32_t&  aClickCount,
                         const int32_t&  aModifiers,
                         const bool&     aIgnoreRootScrollFrame);
    virtual bool RecvHandleTextEvent(const nsString& commit, const nsString& preEdit);
    virtual bool RecvHandleKeyPressEvent(const int& domKeyCode, const int& gmodifiers, const int& charCode);
    virtual bool RecvHandleKeyReleaseEvent(const int& domKeyCode, const int& gmodifiers, const int& charCode);
    virtual bool RecvInputDataTouchEvent(const mozilla::MultiTouchInput&, const gfxSize& res, const gfxPoint& diff);
    virtual bool RecvInputDataTouchMoveEvent(const mozilla::MultiTouchInput&, const gfxSize& res, const gfxPoint& diff);

    virtual bool RecvAsyncMessage(const nsString& aMessage,
                                  const nsString& aData);

private:
    void InitGeckoWindow();
    EmbedLiteAppThreadChild* AppChild();

    uint32_t mId;
    uint64_t mOuterId;
    nsCOMPtr<nsIWidget> mWidget;
    nsCOMPtr<nsIWebBrowser> mWebBrowser;
    nsCOMPtr<nsIWebBrowserChrome> mChrome;
    nsCOMPtr<nsIDOMWindow> mDOMWindow;
    nsCOMPtr<nsIWebNavigation> mWebNavigation;
    WebBrowserChrome* mBChrome;
    gfxSize mViewSize;

    RefPtr<EmbedLiteViewScrolling> mScrolling;
    friend class TabChildHelper;
    friend class EmbedLiteViewScrolling;
    friend class EmbedLiteAppService;

    nsCOMPtr<TabChildHelper> mHelper;
    bool mDispatchSynthMouseEvents;
    bool mHadResizeSinceLastFrameUpdate;
    bool mIMEComposing;

    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteViewThreadChild);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_VIEW_EMBED_THREAD_CHILD_H
