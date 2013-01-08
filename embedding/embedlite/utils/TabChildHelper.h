/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __TabChildHelper_h_
#define __TabChildHelper_h_

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "FrameMetrics.h"
#include "nsFrameMessageManager.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIWebNavigation.h"
#include "nsITabChild.h"
#include "InputData.h"

namespace mozilla {
namespace embedlite {

class TabChildHelper;
class EmbedTabChildGlobal : public nsDOMEventTargetHelper,
                            public nsIContentFrameMessageManager,
                            public nsIScriptObjectPrincipal,
                            public nsIScriptContextPrincipal,
                            public nsITabChild
{
public:
  EmbedTabChildGlobal(TabChildHelper* aTabChild);
  void Init();
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSITABCHILD
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(EmbedTabChildGlobal, nsDOMEventTargetHelper)
  NS_FORWARD_SAFE_NSIMESSAGELISTENERMANAGER(mMessageManager)
  NS_FORWARD_SAFE_NSIMESSAGESENDER(mMessageManager)
  NS_IMETHOD SendSyncMessage(const nsAString& aMessageName,
                             const jsval& aObject,
                             JSContext* aCx,
                             uint8_t aArgc,
                             jsval* aRetval)
  {
      return mMessageManager
          ? mMessageManager->SendSyncMessage(aMessageName, aObject, aCx, aArgc, aRetval)
          : NS_ERROR_NULL_POINTER;
  }
  NS_IMETHOD GetContent(nsIDOMWindow** aContent);
  NS_IMETHOD GetDocShell(nsIDocShell** aDocShell);
  NS_IMETHOD Dump(const nsAString& aStr)
  {
      return mMessageManager ? mMessageManager->Dump(aStr) : NS_OK;
  }
  NS_IMETHOD PrivateNoteIntentionalCrash();
  NS_IMETHOD Btoa(const nsAString& aBinaryData,
                  nsAString& aAsciiBase64String);
  NS_IMETHOD Atob(const nsAString& aAsciiString,
                  nsAString& aBinaryData);

  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              bool aUseCapture)
  {
      // By default add listeners only for trusted events!
      return nsDOMEventTargetHelper::AddEventListener(aType, aListener,
                                                      aUseCapture, false, 2);
  }
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              bool aUseCapture, bool aWantsUntrusted,
                              uint8_t optional_argc)
  {
      return nsDOMEventTargetHelper::AddEventListener(aType, aListener,
                                                      aUseCapture,
                                                      aWantsUntrusted,
                                                      optional_argc);
  }

  virtual nsIScriptObjectPrincipal* GetObjectPrincipal() { return this; }
  virtual JSContext* GetJSContextForEventHandlers();
  virtual nsIPrincipal* GetPrincipal();

  TabChildHelper* mTabChild;
    nsRefPtr<nsFrameMessageManager> mMessageManager;
};

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
    // Wrapper for nsIDOMWindowUtils.setCSSViewport(). This updates some state
    // variables local to this class before setting it.
    void SetCSSViewport(float aX, float aY);

    // Recalculates the display state, including the CSS
    // viewport. This should be called whenever we believe the
    // viewport data on a document may have changed. If it didn't
    // change, this function doesn't do anything.  However, it should
    // not be called all the time as it is fairly expensive.
    void HandlePossibleViewportChange();

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

    bool RecvAsyncMessage(const nsString& aMessage,
                          const nsString& aData);

protected:
    nsIWidget* GetWidget(nsPoint* aOffset);
    nsPresContext* GetPresContext();
    nsEventStatus DispatchWidgetEvent(nsGUIEvent& event);
    // Sends a simulated mouse event from a touch event for compatibility.
    void DispatchSynthesizedMouseEvent(const nsTouchEvent& aEvent);
    bool ConvertMutiTouchInputToEvent(const mozilla::MultiTouchInput& aData,
                                      const gfxSize& res, nsTouchEvent& aEvent);

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

