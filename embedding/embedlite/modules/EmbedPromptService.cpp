/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedPromptService"

#include "EmbedPromptService.h"

#include "nsStringGlue.h"
#include "EmbedLog.h"
#include "nsIAuthPrompt.h"
#include "EmbedLiteViewThreadChild.h"
#include "EmbedLiteAppThreadChild.h"
#include "nsIDOMWindowUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsContentUtils.h"

// Prompt Factory Implementation

using namespace mozilla::embedlite;

EmbedLiteViewPromptResponse::EmbedLiteViewPromptResponse(nsIDOMWindow* aWin)
  : mWin(aWin)
  , confirm(false)
  , checkvalue(false)
{
}

EmbedLiteViewPromptResponse::~EmbedLiteViewPromptResponse()
{
}


EmbedPromptFactory::EmbedPromptFactory()
{
}

EmbedPromptFactory::~EmbedPromptFactory()
{
}

NS_IMPL_ISUPPORTS1(EmbedPromptFactory, nsIPromptFactory)


NS_IMETHODIMP
EmbedPromptFactory::GetPrompt(nsIDOMWindow* aParent, const nsIID& iid, void **result)
{
    if (iid.Equals(NS_GET_IID(nsIAuthPrompt)) ||
        iid.Equals(NS_GET_IID(nsIAuthPrompt2))) {
        EmbedLiteViewThreadChild* view = EmbedLiteAppThreadChild::GetInstance()->ModulesService()->GetViewForWindow(aParent);
        EmbedAuthPromptService* service = new EmbedAuthPromptService(view, aParent);
        *result = service;
        NS_ADDREF(service);
    } else if (iid.Equals(NS_GET_IID(nsIPrompt))) {
        EmbedLiteViewThreadChild* view = EmbedLiteAppThreadChild::GetInstance()->ModulesService()->GetViewForWindow(aParent);
        EmbedPromptService* service = new EmbedPromptService(view, aParent);
        *result = service;
        NS_ADDREF(service);
    }

    return NS_OK;
}

// Prompt Service Implementation

EmbedPromptService::EmbedPromptService(EmbedLiteViewThreadChild* aView, nsIDOMWindow* aWin)
  : mView(aView)
  , mWin(aWin)
{
}

EmbedPromptService::~EmbedPromptService()
{
}

NS_IMPL_ISUPPORTS1(EmbedPromptService, nsIPrompt)

NS_IMETHODIMP
EmbedPromptService::Alert(const PRUnichar* aDialogTitle, 
                          const PRUnichar* aDialogText)
{
    AlertCheck(aDialogTitle, aDialogText, nullptr, nullptr);
    return NS_OK;
}

NS_IMETHODIMP
EmbedPromptService::AlertCheck(const PRUnichar* aDialogTitle,
                               const PRUnichar* aDialogText,
                               const PRUnichar* aCheckMsg, bool* aCheckValue)
{
    uint64_t outerWindowID = 0;
    nsCxPusher pusher;
    pusher.PushNull();
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(mWin);
    NS_ENSURE_TRUE(utils, NS_ERROR_FAILURE);
    utils->GetOuterWindowID(&outerWindowID);
    RefPtr<EmbedLiteViewPromptResponse> resp = new EmbedLiteViewPromptResponse(mWin);
    resp->mWin = mWin;
    if (aCheckMsg && aCheckValue) {
        mView->SendAlert(nsDependentString(aDialogTitle), nsDependentString(aDialogText), nsDependentString(aCheckMsg), *aCheckValue, outerWindowID);
    } else {
        mView->SendAlert(nsDependentString(aDialogTitle), nsDependentString(aDialogText), nsString(), false, outerWindowID);
    }
    mView->WaitForPromptResult(resp);
    if (aCheckValue) {
        *aCheckValue = resp->checkvalue;
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedPromptService::Confirm(const PRUnichar* aDialogTitle,
                            const PRUnichar* aDialogText, bool* aConfirm)
{
    ConfirmCheck(aDialogTitle,
                 aDialogText, nullptr, nullptr, aConfirm);
    return NS_OK;
}

NS_IMETHODIMP
EmbedPromptService::ConfirmCheck(const PRUnichar* aDialogTitle,
                                 const PRUnichar* aDialogText,
                                 const PRUnichar* aCheckMsg,
                                 bool* aCheckValue, bool* aConfirm)
{
    uint64_t outerWindowID = 0;
    nsCxPusher pusher;
    pusher.PushNull();
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(mWin);
    NS_ENSURE_TRUE(utils, NS_ERROR_FAILURE);
    utils->GetOuterWindowID(&outerWindowID);
    RefPtr<EmbedLiteViewPromptResponse> resp = new EmbedLiteViewPromptResponse(mWin);
    resp->mWin = mWin;
    if (aCheckMsg && aCheckValue) {
        mView->SendConfirm(nsDependentString(aDialogTitle), nsDependentString(aDialogText), nsDependentString(aCheckMsg), *aCheckValue, outerWindowID);
    } else {
        mView->SendConfirm(nsDependentString(aDialogTitle), nsDependentString(aDialogText), nsString(), false, outerWindowID);
    }
    mView->WaitForPromptResult(resp);
    if (aCheckValue) {
        *aCheckValue = resp->checkvalue;
    }
    if (aConfirm) {
        *aConfirm = resp->confirm;
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedPromptService::ConfirmEx(const PRUnichar* aDialogTitle,
                              const PRUnichar* aDialogText,
                              PRUint32 aButtonFlags,
                              const PRUnichar* aButton0Title,
                              const PRUnichar* aButton1Title,
                              const PRUnichar* aButton2Title,
                              const PRUnichar* aCheckMsg, bool* aCheckValue,
                              PRInt32* aRetVal)
{
    LOGNI();
    return NS_OK;
}

NS_IMETHODIMP
EmbedPromptService::Prompt(const PRUnichar* aDialogTitle,
                           const PRUnichar* aDialogText, PRUnichar** aValue,
                           const PRUnichar* aCheckMsg, bool* aCheckValue,
                           bool* aConfirm)
{
    uint64_t outerWindowID = 0;
    nsCxPusher pusher;
    pusher.PushNull();
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(mWin);
    NS_ENSURE_TRUE(utils, NS_ERROR_FAILURE);
    utils->GetOuterWindowID(&outerWindowID);
    RefPtr<EmbedLiteViewPromptResponse> resp = new EmbedLiteViewPromptResponse(mWin);
    resp->mWin = mWin;
    mView->SendPrompt(nsDependentString(aDialogTitle),
                      nsDependentString(aDialogText),
                      aValue ? nsDependentString(*aValue) : nsString(),
                      aCheckMsg ? nsDependentString(aCheckMsg) : nsString(),
                      aCheckValue ? *aCheckValue : false, outerWindowID);
    mView->WaitForPromptResult(resp);
    if (aCheckValue) {
        *aCheckValue = resp->checkvalue;
    }
    if (aConfirm) {
        *aConfirm = resp->confirm;
    }
    if (aValue) {
        if (*aValue)
            NS_Free(*aValue);
        *aValue = ToNewUnicode(resp->retVal);
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedPromptService::PromptUsernameAndPassword(const PRUnichar* aDialogTitle,
                                              const PRUnichar* aDialogText,
                                              PRUnichar** aUsername,
                                              PRUnichar** aPassword,
                                              const PRUnichar* aCheckMsg,
                                              bool* aCheckValue,
                                              bool* aConfirm)
{
    LOGNI();
    return NS_OK;
}

NS_IMETHODIMP
EmbedPromptService::PromptPassword(const PRUnichar* aDialogTitle,
                                   const PRUnichar* aDialogText,
                                   PRUnichar** aPassword,
                                   const PRUnichar* aCheckMsg,
                                   bool* aCheckValue, bool* aConfirm)
{
    LOGNI();
    return NS_OK;
}

NS_IMETHODIMP
EmbedPromptService::Select(const PRUnichar* aDialogTitle,
                           const PRUnichar* aDialogText, PRUint32 aCount,
                           const PRUnichar** aSelectList, PRInt32* outSelection,
                           bool* aConfirm)
{
    LOGNI();
    return NS_OK;
}


// Prompt Auth Implementation

EmbedAuthPromptService::EmbedAuthPromptService(EmbedLiteViewThreadChild* aView, nsIDOMWindow* aWin)
  : mView(aView)
  , mWin(aWin)
{
}

EmbedAuthPromptService::~EmbedAuthPromptService()
{
}

NS_IMPL_ISUPPORTS1(EmbedAuthPromptService, nsIAuthPrompt2)

NS_IMETHODIMP
EmbedAuthPromptService::PromptAuth(nsIChannel* aChannel,
                                   uint32_t level,
                                   nsIAuthInformation* authInfo,
                                   bool *_retval)
{
    LOGNI();
    return NS_OK;
}

NS_IMETHODIMP
EmbedAuthPromptService::AsyncPromptAuth(nsIChannel* aChannel, 
                                        nsIAuthPromptCallback* aCallback,
                                        nsISupports *aContext, uint32_t level,
                                        nsIAuthInformation *authInfo,
                                        nsICancelable * *_retval)
{
    LOGNI();
    return NS_OK;
}
