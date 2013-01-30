/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EmbedChromeListener.h"

#include "nsServiceManagerUtils.h"
#include "nsIObserverService.h"
#include "mozilla/embedlite/EmbedLog.h"

#include "nsStringGlue.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMWindow.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEvent.h"
#include "nsPIDOMWindow.h"


EmbedChromeListener::EmbedChromeListener()
{
    LOGT();
}

EmbedChromeListener::~EmbedChromeListener()
{
    LOGT();
}

NS_IMPL_ISUPPORTS3(EmbedChromeListener, nsIObserver, nsIDOMEventListener, nsSupportsWeakReference)

nsresult
EmbedChromeListener::Init()
{
    nsresult rv;

    nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);

    if (observerService) {
        rv = observerService->AddObserver(this,
                                          "domwindowopened",
                                          true);
        rv = observerService->AddObserver(this,
                                          "domwindowclosed",
                                          true);
        rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                          false);
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedChromeListener::Observe(nsISupports *aSubject,
                             const char *aTopic,
                             const PRUnichar *aData)
{
    nsresult rv;
    if (!strcmp(aTopic, "domwindowopened")) {
        nsCOMPtr<nsIDOMWindow> win = do_QueryInterface(aSubject, &rv);
        NS_ENSURE_SUCCESS(rv, NS_OK);
        LOGT("WindowOpened: %p", win.get());
        nsCOMPtr<nsPIDOMWindow> pidomWindow = do_GetInterface(win);
        NS_ENSURE_TRUE(pidomWindow, NS_ERROR_FAILURE);
        nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(pidomWindow->GetChromeEventHandler());
        NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);
        target->AddEventListener(NS_LITERAL_STRING("DOMTitleChanged"), this,  PR_FALSE);
    } else if (!strcmp(aTopic, "domwindclosed")) {
        nsCOMPtr<nsIDOMWindow> win = do_QueryInterface(aSubject, &rv);
        NS_ENSURE_SUCCESS(rv, NS_OK);
        LOGT("WindowClosed: %p", win.get());
        nsCOMPtr<nsPIDOMWindow> pidomWindow = do_GetInterface(win);
        NS_ENSURE_TRUE(pidomWindow, NS_ERROR_FAILURE);
        nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(pidomWindow->GetChromeEventHandler());
        NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);
        target->RemoveEventListener(NS_LITERAL_STRING("DOMTitleChanged"), this,  PR_FALSE);
    } else {
        LOGT("obj:%p, top:%s", aSubject, aTopic);
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedChromeListener::HandleEvent(nsIDOMEvent* aEvent)
{
    nsString type;
    if (aEvent) {
        aEvent->GetType(type);
    }
    LOGT("Event:'%s'", NS_ConvertUTF16toUTF8(type).get());
    return NS_OK;
}
