/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __EmbedTabChildGlobal_h_
#define __EmbedTabChildGlobal_h_

#include "nsFrameMessageManager.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsITabChild.h"

namespace mozilla {
namespace embedlite {

class TabChildHelper;
class EmbedTabChildGlobal : public nsDOMEventTargetHelper,
                            public nsIContentFrameMessageManager,
                            public nsIScriptObjectPrincipal,
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
                             jsval* aRetval) {
    return mMessageManager
           ? mMessageManager->SendSyncMessage(aMessageName, aObject, aCx, aArgc, aRetval)
           : NS_ERROR_NULL_POINTER;
  }
  NS_IMETHOD GetContent(nsIDOMWindow** aContent);
  NS_IMETHOD GetDocShell(nsIDocShell** aDocShell);
  NS_IMETHOD Dump(const nsAString& aStr) {
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
  using nsDOMEventTargetHelper::AddEventListener;
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

  virtual nsIScriptObjectPrincipal* GetObjectPrincipal() {
    return this;
  }
  virtual JSContext* GetJSContextForEventHandlers();
  virtual nsIPrincipal* GetPrincipal();

  TabChildHelper* mTabChild;
  nsRefPtr<nsFrameMessageManager> mMessageManager;
};

}
}

#endif

