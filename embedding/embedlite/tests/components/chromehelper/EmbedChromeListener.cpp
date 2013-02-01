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
#include "nsIEmbedLiteJSON.h"
#include "nsComponentManagerUtils.h"
#include "nsIVariant.h"
#include "nsHashPropertyBag.h"
#include "nsIDOMWindowUtils.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMPopupBlockedEvent.h"
#include "nsIDOMPageTransitionEvent.h"
#include "nsIFocusManager.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWebNavigation.h"

#define MOZ_DOMTitleChanged "DOMTitleChanged"
#define MOZ_DOMContentLoaded "DOMContentLoaded"
#define MOZ_DOMLinkAdded "DOMLinkAdded"
#define MOZ_DOMWillOpenModalDialog "DOMWillOpenModalDialog"
#define MOZ_DOMModalDialogClosed "DOMModalDialogClosed"
#define MOZ_DOMWindowClose "DOMWindowClose"
#define MOZ_DOMPopupBlocked "DOMPopupBlocked"
#define MOZ_pageshow "pageshow"
#define MOZ_pagehide "pagehide"
#define MOZ_DOMMetaAdded "DOMMetaAdded"

EmbedChromeListener::EmbedChromeListener()
  : mWindowCounter(0)
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
        NS_ENSURE_SUCCESS(rv, rv);
        rv = observerService->AddObserver(this,
                                          "domwindowclosed",
                                          true);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                          false);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return rv;
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
        WindowCreated(win);
    } else if (!strcmp(aTopic, "domwindclosed")) {
        nsCOMPtr<nsIDOMWindow> win = do_QueryInterface(aSubject, &rv);
        NS_ENSURE_SUCCESS(rv, NS_OK);
        WindowDestroyed(win);
    } else {
        LOGT("obj:%p, top:%s", aSubject, aTopic);
    }

    return NS_OK;
}

void
EmbedChromeListener::WindowCreated(nsIDOMWindow* aWin)
{
    LOGT("WindowOpened: %p", aWin);
    nsCOMPtr<nsPIDOMWindow> pidomWindow = do_GetInterface(aWin);
    NS_ENSURE_TRUE(pidomWindow, );
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(pidomWindow->GetChromeEventHandler());
    NS_ENSURE_TRUE(target, );
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMTitleChanged), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMContentLoaded), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMLinkAdded), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMWillOpenModalDialog), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMModalDialogClosed), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMWindowClose), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMPopupBlocked), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_pageshow), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_pagehide), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMMetaAdded), this,  PR_FALSE);
    mWindowCounter++;
    if (!mService) {
        mService = do_GetService("@mozilla.org/embedlite-app-service;1");
    }
}

void
EmbedChromeListener::WindowDestroyed(nsIDOMWindow* aWin)
{
    LOGT("WindowClosed: %p", aWin);
    nsCOMPtr<nsPIDOMWindow> pidomWindow = do_GetInterface(aWin);
    NS_ENSURE_TRUE(pidomWindow, );
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(pidomWindow->GetChromeEventHandler());
    NS_ENSURE_TRUE(target, );
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMTitleChanged), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMContentLoaded), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMLinkAdded), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMWillOpenModalDialog), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMModalDialogClosed), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMWindowClose), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMPopupBlocked), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_pageshow), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_pagehide), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMMetaAdded), this,  PR_FALSE);
    mWindowCounter--;
    if (!mWindowCounter) {
        mService = nullptr;
    }
}

nsresult
GetDOMWindowByNode(nsIDOMNode *aNode, nsIDOMWindow **aDOMWindow)
{
    nsresult rv;
    nsCOMPtr<nsIDOMDocument> ctDoc = do_QueryInterface(aNode, &rv);
    NS_ENSURE_SUCCESS(rv , rv);
    nsCOMPtr<nsIDOMWindow> targetWin;
    rv = ctDoc->GetDefaultView(getter_AddRefs(targetWin));
    NS_ENSURE_SUCCESS(rv , rv);
    *aDOMWindow = targetWin.forget().get();
    return rv;
}

NS_IMETHODIMP
GetTopWindow(nsIDOMWindow* aWin, nsIDOMWindow **aDOMWindow)
{
    nsCOMPtr<nsIDOMWindow> window;
    nsCOMPtr<nsIWebNavigation> navNav(do_GetInterface(aWin));
    nsCOMPtr<nsIDocShellTreeItem> navItem(do_QueryInterface(navNav));
    NS_ENSURE_TRUE(navItem, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    navItem->GetRootTreeItem(getter_AddRefs(rootItem));
    nsCOMPtr<nsIDOMWindow> rootWin(do_GetInterface(rootItem));
    NS_ENSURE_TRUE(rootWin, NS_ERROR_FAILURE);
    rootWin->GetTop(getter_AddRefs(window));
    *aDOMWindow = window.forget().get();
    return NS_OK;
}


NS_IMETHODIMP
EmbedChromeListener::HandleEvent(nsIDOMEvent* aEvent)
{
    nsresult rv;
    nsString type;
    if (aEvent) {
        aEvent->GetType(type);
    }
    LOGT("Event:'%s'", NS_ConvertUTF16toUTF8(type).get());

    nsString messageName;
    nsString message;
    // Just simple property bag support still
    nsCOMPtr<nsIEmbedLiteJSON> json = do_GetService("@mozilla.org/embedlite-json;1");
    nsCOMPtr<nsIWritablePropertyBag2> root;
    json->CreateObject(getter_AddRefs(root));

    nsCOMPtr<nsIDOMEventTarget> eventTarget;
    rv = aEvent->GetTarget(getter_AddRefs(eventTarget));
    NS_ENSURE_SUCCESS(rv , rv);
    nsCOMPtr<nsIDOMNode> eventNode = do_QueryInterface(eventTarget, &rv);
    NS_ENSURE_SUCCESS(rv , rv);
    nsCOMPtr<nsIDOMWindow> window;
    rv = GetDOMWindowByNode(eventNode, getter_AddRefs(window));
    nsCOMPtr<nsIDOMWindow> docWin = do_GetInterface(window);

    uint32_t winid;
    mService->GetIDByWindow(window, &winid);
    NS_ENSURE_TRUE(winid , NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);

    if (type.EqualsLiteral(MOZ_DOMMetaAdded)) {
        messageName.AssignLiteral("chrome:metaadded");
    } else if (type.EqualsLiteral(MOZ_DOMTitleChanged)) {
        nsCOMPtr<nsIDOMDocument> ctDoc;
        window->GetDocument(getter_AddRefs(ctDoc));
        nsString title;
        ctDoc->GetTitle(title);
        messageName.AssignLiteral("chrome:title");
        root->SetPropertyAsAString(NS_LITERAL_STRING("title"), title);
    } else if (type.EqualsLiteral(MOZ_DOMContentLoaded)) {
        nsCOMPtr<nsIDOMDocument> ctDoc;
        docWin->GetDocument(getter_AddRefs(ctDoc));
        nsString docURI;
        ctDoc->GetDocumentURI(docURI);
        if (!docURI.EqualsLiteral("about:blank")) {
            messageName.AssignLiteral("chrome:contentloaded");
            root->SetPropertyAsAString(NS_LITERAL_STRING("docuri"), docURI);
        }
        // Need send session history from here
    } else if (type.EqualsLiteral(MOZ_DOMLinkAdded)) {
        nsCOMPtr<nsIDOMEventTarget> origTarget;
        aEvent->GetOriginalTarget(getter_AddRefs(origTarget));
        nsCOMPtr<nsIDOMHTMLLinkElement> disabledIface = do_QueryInterface(origTarget);
        nsString href;
        bool disabled = true;
        disabledIface->GetDisabled(&disabled);
        if (!disabledIface || disabled) {
            return NS_OK;
        }
        disabledIface->GetHref(href);
        uint64_t currentInnerWindowID = 0;
        utils->GetCurrentInnerWindowID(&currentInnerWindowID);
        nsCOMPtr<nsIDOMDocument> ctDoc;
        docWin->GetDocument(getter_AddRefs(ctDoc));
        nsString charset, title, rel, type;
        ctDoc->GetCharacterSet(charset);
        ctDoc->GetTitle(title);
        disabledIface->GetRel(rel);
        disabledIface->GetType(type);
        nsString sizes;
        nsCOMPtr<nsIDOMElement> element = do_QueryInterface(origTarget);
        bool hasSizesAttr = false;
        if (NS_SUCCEEDED(element->HasAttribute(NS_LITERAL_STRING("sizes"), &hasSizesAttr)) && hasSizesAttr) {
            element->GetAttribute(NS_LITERAL_STRING("sizes"), sizes);
        }
        messageName.AssignLiteral("chrome:linkadded");
        root->SetPropertyAsAString(NS_LITERAL_STRING("href"), href);
        root->SetPropertyAsAString(NS_LITERAL_STRING("charset"), charset);
        root->SetPropertyAsAString(NS_LITERAL_STRING("title"), title);
        root->SetPropertyAsAString(NS_LITERAL_STRING("rel"), rel);
        root->SetPropertyAsAString(NS_LITERAL_STRING("sizes"), sizes);
        root->SetPropertyAsAString(NS_LITERAL_STRING("get"), type);
    } else if (type.EqualsLiteral(MOZ_DOMWillOpenModalDialog) ||
               type.EqualsLiteral(MOZ_DOMModalDialogClosed) ||
               type.EqualsLiteral(MOZ_DOMWindowClose)) {
        messageName.AssignLiteral("chrome:winopenclose");
        root->SetPropertyAsAString(NS_LITERAL_STRING("type"), type);
    } else if (type.EqualsLiteral(MOZ_DOMPopupBlocked)) {
        uint64_t outerWindowID = 0;
        utils->GetOuterWindowID(&outerWindowID);
        nsCOMPtr<nsIDOMPopupBlockedEvent> popupEvent = do_QueryInterface(aEvent);
        nsCOMPtr<nsIURI> popupUri;
        popupEvent->GetPopupWindowURI(getter_AddRefs(popupUri));
        nsString popupWinFeatures, popupWindowName;
        nsCString spec, origCharset;
        popupUri->GetSpec(spec);
        popupUri->GetOriginCharset(origCharset);
        popupEvent->GetPopupWindowFeatures(popupWinFeatures);
        popupEvent->GetPopupWindowName(popupWindowName);

        messageName.AssignLiteral("chrome:popupblocked");
        root->SetPropertyAsACString(NS_LITERAL_STRING("spec"), spec);
        root->SetPropertyAsACString(NS_LITERAL_STRING("origCharset"), origCharset);
        root->SetPropertyAsAString(NS_LITERAL_STRING("popupWinFeatures"), popupWinFeatures);
        root->SetPropertyAsAString(NS_LITERAL_STRING("popupWindowName"), popupWindowName);
    } else if (type.EqualsLiteral(MOZ_pageshow) ||
               type.EqualsLiteral(MOZ_pagehide)) {
        nsCOMPtr<nsIDOMEventTarget> target;
        aEvent->GetTarget(getter_AddRefs(target));
        nsCOMPtr<nsIDOMDocument> ctDoc = do_QueryInterface(target);
        nsCOMPtr<nsIDOMWindow> targetWin;
        ctDoc->GetDefaultView(getter_AddRefs(targetWin));
        nsCOMPtr<nsIDOMWindow> docWin;
        GetTopWindow(targetWin, getter_AddRefs(docWin));
        if (targetWin != docWin) {
            return NS_OK;
        }
        nsCOMPtr<nsIDOMWindowUtils> tutils = do_GetInterface(targetWin);
        uint64_t outerWindowID = 0, tinnerID = 0;
        tutils->GetOuterWindowID(&outerWindowID);
        tutils->GetCurrentInnerWindowID(&tinnerID);
        int32_t innerWidth, innerHeight;
        docWin->GetInnerWidth(&innerWidth);
        docWin->GetInnerHeight(&innerHeight);
        nsCOMPtr<nsIDOMPageTransitionEvent> transEvent = do_QueryInterface(aEvent);
        bool persisted = false;
        transEvent->GetPersisted(&persisted);
        messageName.AssignLiteral("chrome:");
        messageName.Append(type);
        root->SetPropertyAsBool(NS_LITERAL_STRING("persisted"), persisted);
    } else {
        return NS_OK;
    }

    nsString outStr;
    json->CreateJSON(root, message);
    mService->SendAsyncMessage(winid, messageName, message);

    return NS_OK;
}
