/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedPromptService"
#include "EmbedLog.h"

#include "EmbedPromptService.h"

#include "nsStringGlue.h"
#include "nsIAuthPrompt.h"
#include "EmbedLiteViewThreadChild.h"
#include "EmbedLiteAppThreadChild.h"
#include "nsIDOMWindowUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsContentUtils.h"
#include "nsIAuthInformation.h"
#include "nsICancelable.h"
#include "nsIAuthPromptCallback.h"
#include "nsIIOService.h"
#include "nsNetCID.h"
#include "nsIProtocolHandler.h"

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
    nsCxPusher pusher;
    pusher.PushNull();
    uint64_t outerWindowID = 0;
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
        printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

EmbedAuthPromptService::~EmbedAuthPromptService()
{
        printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
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

class nsAuthCancelableConsumer MOZ_FINAL : public nsICancelable
{
public:
    NS_DECL_ISUPPORTS

    nsAuthCancelableConsumer(nsIAuthPromptCallback* aCallback,
                             nsISupports *aContext)
        : mCallback(aCallback)
        , mContext(aContext)
    {
        NS_ASSERTION(mCallback, "null callback");
        printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    }

    ~nsAuthCancelableConsumer()
    {
        printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    }

    NS_IMETHOD Cancel(nsresult reason)
    {
        NS_ENSURE_ARG(NS_FAILED(reason));

        // If we've already called DoCallback then, nothing more to do.
        if (mCallback) {
            mCallback->OnAuthCancelled(mContext, false);
        }
        mCallback = nullptr;
        mContext = nullptr;
        return NS_OK;
    }

    nsCOMPtr<nsIAuthPromptCallback> mCallback;
    nsCOMPtr<nsISupports> mContext;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsAuthCancelableConsumer, nsICancelable);

static nsCString getFormattedHostname(nsIURI* uri)
{
    nsCString scheme;
    uri->GetScheme(scheme);
    nsCString host;
    uri->GetHost(host);
    nsCString hostname(scheme + nsCString("://") + host);

    // If the URI explicitly specified a port, only include it when
    // it's not the default. (We never want "http://foo.com:80")
    int32_t port = -1;
    uri->GetPort(&port);
    if (port != -1) {
        int32_t handlerPort = -1;
        nsCOMPtr<nsIIOService> ioService = do_GetService(NS_IOSERVICE_CONTRACTID);
        if (ioService) {
            nsCOMPtr<nsIProtocolHandler> handler;
            ioService->GetProtocolHandler(scheme.get(), getter_AddRefs(handler));
            if (handler) {
                handler->GetDefaultPort(&handlerPort);
            }
        }
        if (port != handlerPort) {
            hostname += ":" + port;
        }
    }
    return hostname;
}

static nsresult
getAuthTarget(nsIChannel* aChannel, nsIAuthInformation *authInfo, nsCString& hostname, nsCString& realm)
{
    nsresult rv;
    nsCOMPtr<nsIURI> uri;
    rv = aChannel->GetURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
    hostname = getFormattedHostname(uri);
    // If a HTTP WWW-Authenticate header specified a realm, that value
    // will be available here. If it wasn't set or wasn't HTTP, we'll use
    // the formatted hostname instead.
    nsString ut16realm;
    if (NS_FAILED(authInfo->GetRealm(ut16realm)) || ut16realm.IsEmpty()) {
        realm = hostname;
    } else {
        realm = NS_ConvertUTF16toUTF8(ut16realm);
    }
    return NS_OK;
}

class EmbedAuthRunnable : public nsRunnable
{
public:
    EmbedAuthRunnable(EmbedAsyncAuthPrompt* aPrompt)
      : mPrompt(aPrompt)
    {
        printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    }
    virtual ~EmbedAuthRunnable()
    {
        printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    }
    NS_IMETHOD Run();
    EmbedAsyncAuthPrompt* mPrompt;
};

NS_IMETHODIMP
EmbedAuthRunnable::Run()
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    if (!mPrompt->mView) {
        return NS_ERROR_FAILURE;
    }
    nsCString hostname, httpRealm;
    NS_ENSURE_SUCCESS(getAuthTarget(mPrompt->mChannel, mPrompt->mAuthInfo, hostname, httpRealm), NS_ERROR_FAILURE);
    nsString username;
    nsresult rv;
    uint32_t authInfoFlags;
    rv = mPrompt->mAuthInfo->GetFlags(&authInfoFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    bool isOnlyPassword = !!(authInfoFlags & nsIAuthInformation::ONLY_PASSWORD);
    mPrompt->mAuthInfo->GetUsername(username);

    nsRefPtr<EmbedLiteViewPromptResponse> resp = new EmbedLiteViewPromptResponse(mPrompt->mWin);
    nsCxPusher pusher;

    pusher.PushNull();
    uint64_t outerWindowID = 0;
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(mPrompt->mWin);
    NS_ENSURE_TRUE(utils, NS_ERROR_FAILURE);
    utils->GetOuterWindowID(&outerWindowID);
    mPrompt->mView->SendAuthentificationRequired(outerWindowID, hostname, httpRealm, username, isOnlyPassword);

    mPrompt->mView->WaitForPromptResult(resp);
    mPrompt->mService->DoResponseAsyncPrompt(mPrompt, resp->confirm, NS_ConvertUTF16toUTF8(resp->username), NS_ConvertUTF16toUTF8(resp->password));
    delete mPrompt;
    mPrompt = nullptr;

    return NS_OK;
}

NS_IMETHODIMP
EmbedAuthPromptService::AsyncPromptAuth(nsIChannel* aChannel,
                                        nsIAuthPromptCallback* aCallback,
                                        nsISupports *aContext, uint32_t level,
                                        nsIAuthInformation *authInfo,
                                        nsICancelable * *_retval)
{
    nsCxPusher pusher;
    pusher.PushNull();

    // The cases that we don't support now.
    nsresult rv;
    uint32_t authInfoFlags;
    rv = authInfo->GetFlags(&authInfoFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if ((authInfoFlags & nsIAuthInformation::AUTH_PROXY) &&
        (authInfoFlags & nsIAuthInformation::ONLY_PASSWORD)) {
        NS_ERROR("Not Implemented");
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIInterfaceRequestor> notificationCallbacks;
    rv = aChannel->GetNotificationCallbacks(getter_AddRefs(notificationCallbacks));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsILoadContext> loadContext = do_GetInterface(notificationCallbacks);
    nsCOMPtr<nsIDOMWindow> topWindow;
    rv = loadContext->GetTopWindow(getter_AddRefs(topWindow));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsAuthCancelableConsumer> consumer = new nsAuthCancelableConsumer(aCallback, aContext);

    nsCString hostname, httpRealm;
    NS_ENSURE_SUCCESS(getAuthTarget(aChannel, authInfo, hostname, httpRealm), NS_ERROR_FAILURE);

    nsCString hashKey;
    hashKey.AppendPrintf("%u|%s|%s", level, hostname.get(), httpRealm.get());
    LOGT("host:%s, realm:%s, hash:%s", hostname.get(), httpRealm.get(), hashKey.get());
    EmbedAsyncAuthPrompt* asyncPrompt = asyncPrompts[hashKey.get()];
    if (asyncPrompt) {
        asyncPrompt->consumers.AppendElement(consumer);
        NS_ADDREF(*_retval = consumer);
        return NS_OK;
    }
    asyncPrompt = new EmbedAsyncAuthPrompt(consumer, aChannel, authInfo, level, false, mView);
    asyncPrompt->mWin = mWin;
    asyncPrompt->mHashKey = hashKey;
    asyncPrompt->mService = this;
    asyncPrompts[hashKey.get()] = asyncPrompt;
    DoAsyncPrompt();
    return NS_OK;
}

void
EmbedAuthPromptService::DoAsyncPrompt()
{
    // Find the key of a prompt whose browser element parent does not have
    // async prompt in progress.
    nsCString hashKey;
    std::map<std::string, EmbedAsyncAuthPrompt*>::iterator it;
    for (it = asyncPrompts.begin(); it != asyncPrompts.end(); it++) {
        EmbedAsyncAuthPrompt* asyncPrompt = it->second;
        if (asyncPrompt) {
            if (!asyncPrompt || !asyncPromptInProgress[asyncPrompt->mView]) {
                hashKey = it->first.c_str();
                break;
            }
        }
    }
    // Didn't find an available prompt, so just return.
    if (hashKey.IsEmpty()) {
        return;
    }

    EmbedAsyncAuthPrompt* asyncPrompt = asyncPrompts[hashKey.get()];
    nsCString hostname, httpRealm;
    NS_ENSURE_SUCCESS(getAuthTarget(asyncPrompt->mChannel, asyncPrompt->mAuthInfo, hostname, httpRealm), );
    if (asyncPrompt->mView) {
        asyncPromptInProgress[asyncPrompt->mView] = true;
    }
    asyncPrompt->mInProgress = true;
    nsCOMPtr<nsIRunnable> runnable = new EmbedAuthRunnable(asyncPrompt);
    nsIThread *thread = NS_GetCurrentThread();
    if (NS_FAILED(thread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL))) {
        NS_WARNING("Dispatching EmbedAuthRunnable failed.");
    }
}

void
EmbedAuthPromptService::DoResponseAsyncPrompt(EmbedAsyncAuthPrompt* prompt,
                                              const bool& confirmed,
                                              const nsCString& username,
                                              const nsCString& password)
{
    nsresult rv;
    asyncPrompts.erase(prompt->mHashKey.get());
    prompt->mInProgress = false;
    if (prompt->mView) {
        asyncPromptInProgress.erase(prompt->mView);
    }
    // Fill authentication information with username and password provided
    // by user.
    uint32_t flags;
    rv = prompt->mAuthInfo->GetFlags(&flags);
    NS_ENSURE_SUCCESS(rv, );
    if (!username.IsEmpty()) {
        if (flags & nsIAuthInformation::NEED_DOMAIN) {
            // Domain is separated from username by a backslash
            int idx = username.Find("\\");
            if (idx == -1) {
                prompt->mAuthInfo->SetUsername(NS_ConvertUTF8toUTF16(username));
            } else {
                prompt->mAuthInfo->SetDomain(NS_ConvertUTF8toUTF16(nsDependentCSubstring(username, 0, idx)));
                prompt->mAuthInfo->SetUsername(NS_ConvertUTF8toUTF16(nsDependentCSubstring(username, idx + 1)));
            }
        } else {
            prompt->mAuthInfo->SetUsername(NS_ConvertUTF8toUTF16(username));
        }
    }

    if (!password.IsEmpty()) {
        prompt->mAuthInfo->SetPassword(NS_ConvertUTF8toUTF16(password));
    }

    for (unsigned int i = 0; i < prompt->consumers.Length(); i++) {
        nsCOMPtr<nsICancelable> consumerI = prompt->consumers[i];
        nsCOMPtr<nsAuthCancelableConsumer> consumer = do_QueryInterface(consumerI);
        if (!consumer->mCallback) {
            // Not having a callback means that consumer didn't provide it
            // or canceled the notification.
            continue;
        }
        if (confirmed) {
            // printf("Ok, calling onAuthAvailable to finish auth.\n");
            consumer->mCallback->OnAuthAvailable(consumer->mContext, prompt->mAuthInfo);
        } else {
            // printf("Cancelled, calling onAuthCancelled to finish auth.\n");
            consumer->mCallback->OnAuthCancelled(consumer->mContext, true);
        }
    }

    // Process the next prompt, if one is pending.
    DoAsyncPrompt();
}

