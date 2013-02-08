/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __TabChildHelper_h_
#define __TabChildHelper_h_

#include "nsIObserver.h"
#include "FrameMetrics.h"
#include "nsFrameMessageManager.h"
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "InputData.h"
#include "nsDataHashtable.h"

class CancelableTask;
class nsPresContext;

namespace mozilla {
namespace embedlite {

class EmbedTabChildGlobal;
class EmbedLiteViewThreadChild;
class TabChildHelper : public nsIObserver,
                       public nsFrameScriptExecutor,
                       public mozilla::dom::ipc::MessageManagerCallback
{
public:
    TabChildHelper(EmbedLiteViewThreadChild* aView);
    virtual ~TabChildHelper();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    bool RecvUpdateFrame(const mozilla::layers::FrameMetrics& aFrameMetrics);

    JSContext* GetJSContext() { return mCx; }

    nsIWebNavigation* WebNavigation();

    nsIPrincipal* GetPrincipal() { return mPrincipal; }

    virtual bool DoLoadFrameScript(const nsAString& aURL);
    virtual bool DoSendSyncMessage(const nsAString& aMessage,
                                   const mozilla::dom::StructuredCloneData& aData,
                                   InfallibleTArray<nsString>* aJSONRetVal);
    virtual bool DoSendAsyncMessage(const nsAString& aMessage,
                                    const mozilla::dom::StructuredCloneData& aData);
    virtual bool CheckPermission(const nsAString& aPermission);

    bool RecvAsyncMessage(const nsAString& aMessage,
                          const nsAString& aData);

protected:
    nsIWidget* GetWidget(nsPoint* aOffset);
    nsPresContext* GetPresContext();
    void InitEvent(nsGUIEvent& event, nsIntPoint* aPoint = nullptr);
    nsEventStatus DispatchWidgetEvent(nsGUIEvent& event);
    // Sends a simulated mouse event from a touch event for compatibility.
    bool ConvertMutiTouchInputToEvent(const mozilla::MultiTouchInput& aData,
                                      const gfxSize& res, const gfxPoint& diff,
                                      nsTouchEvent& aEvent);
    void DispatchSynthesizedMouseEvent(uint32_t aMsg, uint64_t aTime,
                                       const nsIntPoint& aRefPoint);
    nsEventStatus DispatchSynthesizedMouseEvent(const nsTouchEvent& aEvent);

    void CancelTapTracking();

private:
    bool InitTabChildGlobal();
    void Disconnect();
    void Unload();

    friend class EmbedLiteViewThreadChild;
    EmbedLiteViewThreadChild* mView;
    bool mContentDocumentIsDisplayed;
    mozilla::layers::FrameMetrics mLastMetrics;
    nsIntSize mInnerSize;
    float mOldViewportWidth;
    nsRefPtr<EmbedTabChildGlobal> mTabChildGlobal;
};

}}

#endif

