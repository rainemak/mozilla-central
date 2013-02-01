/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EmbedHistoryListener.h"
#include "nsIURI.h"
//#include "EmbedliteBridge.h"
//#include "Link.h"

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

    printf(">>>>>>Func:%s::%d uri:%s\n", __PRETTY_FUNCTION__, __LINE__, uri.get());

/*
    NS_ConvertUTF8toUTF16 uriString(uri);

    nsTArray<Link*>* list = mListeners.Get(uriString);
    if (! list) {
        list = new nsTArray<Link*>();
        mListeners.Put(uriString, list);
    }
    list->AppendElement(aContent);
    EmbedliteBridge *bridge = EmbedliteBridge::Bridge();
    if (bridge) {
        bridge->CheckURIVisited(uriString);
    }
*/

    return NS_OK;
}

NS_IMETHODIMP
EmbedHistoryListener::UnregisterVisitedCallback(nsIURI *aURI, Link *aContent)
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
  if (!aContent || !aURI)
    return NS_OK;

/*
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
*/
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
    printf(">>>>>>Func:%s::%d uri:%s\n", __PRETTY_FUNCTION__, __LINE__, uri.get());
    return NS_OK;
}

NS_IMETHODIMP
EmbedHistoryListener::SetURITitle(nsIURI *aURI, const nsAString& aTitle)
{
    if (!aURI)
        return NS_OK;

    // we don't do anything with this right now
    nsAutoCString uri;
    nsresult rv = aURI->GetSpec(uri);
    if (NS_FAILED(rv)) return rv;

    printf(">>>>>>Func:%s::%d uri:%s, title:%s\n", __PRETTY_FUNCTION__, __LINE__, uri.get(), NS_ConvertUTF16toUTF8(aTitle).get());
    return NS_OK;
}

NS_IMETHODIMP
EmbedHistoryListener::NotifyVisited(nsIURI *aURI)
{
    if (!aURI)
        return NS_OK;

    nsAutoCString uri;
    nsresult rv = aURI->GetSpec(uri);
    if (NS_FAILED(rv)) return rv;
    printf(">>>>>>Func:%s::%d uri:%s\n", __PRETTY_FUNCTION__, __LINE__, uri.get());
    return NS_OK;
}

NS_IMETHODIMP
EmbedHistoryListener::Run()
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    return NS_OK;
}
