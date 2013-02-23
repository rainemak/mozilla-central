/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EmbedHistoryListener.h"
#include "nsIURI.h"
#include "Link.h"
#include "nsIEmbedLiteJSON.h"

using namespace mozilla;
using mozilla::dom::Link;

NS_IMPL_ISUPPORTS2(EmbedHistoryListener, IHistory, nsIRunnable)

EmbedHistoryListener* EmbedHistoryListener::sHistory = NULL;

/*static*/
EmbedHistoryListener*
EmbedHistoryListener::GetSingleton()
{
  if (!sHistory) {
    sHistory = new EmbedHistoryListener();
    NS_ENSURE_TRUE(sHistory, nullptr);
  }

  NS_ADDREF(sHistory);
  return sHistory;
}

EmbedHistoryListener::EmbedHistoryListener()
{
  mListeners.Init();
}

NS_IMETHODIMP
EmbedHistoryListener::RegisterVisitedCallback(nsIURI *aURI, Link *aContent)
{
    if (!aContent || !aURI)
        return NS_OK;

    nsAutoCString uri;
    nsresult rv = aURI->GetSpec(uri);
    if (NS_FAILED(rv)) return rv;

    NS_ConvertUTF8toUTF16 uriString(uri);

    nsTArray<Link*>* list = mListeners.Get(uriString);
    if (! list) {
        list = new nsTArray<Link*>();
        mListeners.Put(uriString, list);
    }
    list->AppendElement(aContent);

    nsString message;
    // Just simple property bag support still
    nsCOMPtr<nsIEmbedLiteJSON> json = do_GetService("@mozilla.org/embedlite-json;1");
    nsCOMPtr<nsIWritablePropertyBag2> root;
    json->CreateObject(getter_AddRefs(root));
    root->SetPropertyAsACString(NS_LITERAL_STRING("uri"), uri);

    json->CreateJSON(root, message);
    GetService()->SendGlobalAsyncMessage(NS_LITERAL_STRING("history:checkurivisited"), message);

    return NS_OK;
}

NS_IMETHODIMP
EmbedHistoryListener::UnregisterVisitedCallback(nsIURI *aURI, Link *aContent)
{
    if (!aContent || !aURI)
        return NS_OK;

    nsAutoCString uri;
    nsresult rv = aURI->GetSpec(uri);
    if (NS_FAILED(rv)) return rv;
    NS_ConvertUTF8toUTF16 uriString(uri);

    nsTArray<Link*>* list = mListeners.Get(uriString);
    if (! list)
        return NS_OK;

    list->RemoveElement(aContent);
    if (list->IsEmpty()) {
        mListeners.Remove(uriString);
        delete list;
    }
    return NS_OK;
}

NS_IMETHODIMP
EmbedHistoryListener::VisitURI(nsIURI *aURI, nsIURI *aLastVisitedURI, uint32_t aFlags)
{
    if (!aURI)
        return NS_OK;

    if (!(aFlags & VisitFlags::TOP_LEVEL))
        return NS_OK;

    if (aFlags & VisitFlags::REDIRECT_SOURCE)
        return NS_OK;

    if (aFlags & VisitFlags::UNRECOVERABLE_ERROR)
        return NS_OK;


    nsAutoCString uri;
    nsresult rv = aURI->GetSpec(uri);
    if (NS_FAILED(rv)) return rv;

    nsString message;
    // Just simple property bag support still
    nsCOMPtr<nsIEmbedLiteJSON> json = do_GetService("@mozilla.org/embedlite-json;1");
    nsCOMPtr<nsIWritablePropertyBag2> root;
    json->CreateObject(getter_AddRefs(root));
    root->SetPropertyAsACString(NS_LITERAL_STRING("uri"), uri);

    json->CreateJSON(root, message);
    GetService()->SendGlobalAsyncMessage(NS_LITERAL_STRING("history:markurivisited"), message);
    
    return NS_OK;
}

nsIEmbedAppService*
EmbedHistoryListener::GetService()
{
    if (!mService) {
        mService = do_GetService("@mozilla.org/embedlite-app-service;1");
    }
    return mService.get();
}

NS_IMETHODIMP
EmbedHistoryListener::SetURITitle(nsIURI *aURI, const nsAString& aTitle)
{
    if (!aURI)
        return NS_OK;

    // we don't do anything with this right now
#if 0
    nsAutoCString uri;
    nsresult rv = aURI->GetSpec(uri);
    if (NS_FAILED(rv)) return rv;
#endif

    return NS_OK;
}

NS_IMETHODIMP
EmbedHistoryListener::NotifyVisited(nsIURI *aURI)
{
    if (aURI && sHistory) {
        nsAutoCString spec;
        (void)aURI->GetSpec(spec);
        sHistory->mPendingURIs.Push(NS_ConvertUTF8toUTF16(spec));
        NS_DispatchToMainThread(sHistory);
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedHistoryListener::Run()
{
    while (! mPendingURIs.IsEmpty()) {
        nsString uriString = mPendingURIs.Pop();
        nsTArray<Link*>* list = sHistory->mListeners.Get(uriString);
        if (list) {
            for (unsigned int i = 0; i < list->Length(); i++) {
                list->ElementAt(i)->SetLinkState(eLinkState_Visited);
            }
            // as per the IHistory interface contract, remove the
            // Link pointers once they have been notified
            mListeners.Remove(uriString);
            delete list;
        }
    }

    return NS_OK;
}
