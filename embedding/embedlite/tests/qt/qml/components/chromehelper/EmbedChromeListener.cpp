/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EmbedChromeListener.h"

#include "nsServiceManagerUtils.h"
#include "nsIObserverService.h"

EmbedChromeListener::EmbedChromeListener()
{
}

EmbedChromeListener::~EmbedChromeListener()
{
}

NS_IMPL_ISUPPORTS2(EmbedChromeListener, nsIObserver, nsSupportsWeakReference)

nsresult
EmbedChromeListener::Init()
{
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);

    if (observerService) {
        observerService->AddObserver(this,
                                     "outer-window-destroyed",
                                     true);
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedChromeListener::Observe(nsISupports *aSubject,
                             const char *aTopic,
                             const PRUnichar *aData)
{
    return NS_OK;
}
