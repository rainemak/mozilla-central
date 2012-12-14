/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebBrowserChrome.h"
#include "nsIDOMWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShell.h"
#include "nsIWebProgress.h"
#include "nsIDOMEventTarget.h"
#include "nsPIDOMWindow.h"
#include "nsNetUtil.h"
#include "nsIDOMWindowUtils.h"
#include <inttypes.h>
#include "EmbedLog.h"

#define MOZ_AFTER_PAINT_LITERAL "MozAfterPaint"
#define MOZ_DOMContentLoaded "DOMContentLoaded"
#define MOZ_DOMTitleChanged "DOMTitleChanged"
#define MOZ_DOMLinkAdded "DOMLinkAdded"
#define MOZ_DOMWillOpenModalDialog "DOMWillOpenModalDialog"
#define MOZ_DOMModalDialogClosed "DOMModalDialogClosed"
#define MOZ_DOMWindowClose "DOMWindowClose"
#define MOZ_DOMPopupBlocked "DOMPopupBlocked"
#define MOZ_pageshow "pageshow"
#define MOZ_pagehide "pagehide"
#define MOZ_scroll "scroll"
#define MOZ_MozScrolledAreaChanged "MozScrolledAreaChanged"

WebBrowserChrome::WebBrowserChrome()
 : mChromeFlags(0)
 , mIsModal(false)
 , mIsVisible(false)
 , mHandlerAdded(false)
{
    LOGT();
}

WebBrowserChrome::~WebBrowserChrome()
{
    LOGT();
}

NS_IMPL_ISUPPORTS7(WebBrowserChrome,
                   nsIWebBrowserChrome,
                   nsIWebBrowserChromeFocus,
                   nsIInterfaceRequestor,
                   nsIEmbeddingSiteWindow,
                   nsIWebProgressListener,
                   nsIObserver,
                   nsSupportsWeakReference)

NS_IMETHODIMP WebBrowserChrome::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);

    *aInstancePtr = 0;
    if (aIID.Equals(NS_GET_IID(nsIDOMWindow))) {
        if (!mWebBrowser)
            return NS_ERROR_NOT_INITIALIZED;

        return mWebBrowser->GetContentDOMWindow((nsIDOMWindow **)aInstancePtr);
    }
    return QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP WebBrowserChrome::SetStatus(uint32_t /* statusType*/, const PRUnichar* /*status*/)
{
    LOGNI();
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetWebBrowser(nsIWebBrowser * *aWebBrowser)
{
    LOGNI();
    NS_ENSURE_ARG_POINTER(aWebBrowser);
    *aWebBrowser = mWebBrowser;
    NS_IF_ADDREF(*aWebBrowser);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetWebBrowser(nsIWebBrowser * aWebBrowser)
{
    LOGNI();
    mWebBrowser = aWebBrowser;
    SetEventHandler();
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetChromeFlags(uint32_t *aChromeFlags)
{
    LOGNI();
    *aChromeFlags = mChromeFlags;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetChromeFlags(uint32_t aChromeFlags)
{
    LOGNI();
    mChromeFlags = aChromeFlags;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::DestroyBrowserWindow()
{
    LOGNI();
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SizeBrowserTo(int32_t aCX, int32_t aCY)
{
    printf("WebBrowserChrome::%s::%d s[%i,%i]\n", __FUNCTION__, __LINE__, aCX, aCY);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ShowAsModal()
{
    printf("WebBrowserChrome::%s::%d\n", __FUNCTION__, __LINE__);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::IsWindowModal(bool *_retval)
{
    LOGNI();
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = mIsModal;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
    printf("WebBrowserChrome::%s::%d status:%x\n", __FUNCTION__, __LINE__, aStatus);
    return NS_OK;
}

// ----- Progress Listener -----

//*****************************************************************************
// WebBrowserChrome::nsIObserver
//*****************************************************************************

NS_IMETHODIMP
WebBrowserChrome::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;
    printf("subj:%p, top:%s\n", aSubject, aTopic);
    return rv;
}

//*****************************************************************************
// WebBrowserChrome::nsIWebProgressListener
//*****************************************************************************

NS_IMETHODIMP
WebBrowserChrome::OnProgressChange(nsIWebProgress *progress, nsIRequest *request,
                                     int32_t curSelfProgress, int32_t maxSelfProgress,
                                     int32_t curTotalProgress, int32_t maxTotalProgress)
{
    LOGNI();
    return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::OnStateChange(nsIWebProgress *progress, nsIRequest *request,
                                       uint32_t progressStateFlags, nsresult status)
{
    nsCOMPtr<nsIDOMWindow> docWin = do_GetInterface(mWebBrowser);
    nsCOMPtr<nsIDOMWindow> progWin;
    progress->GetDOMWindow(getter_AddRefs(progWin));
    if (progWin != docWin) {
        return NS_OK;
    }

    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mWebBrowser);
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);
    if (!utils) {
        NS_WARNING("window Utils are null");
        return NS_OK;
    }
    uint64_t currentInnerWindowID = 0;
    utils->GetCurrentInnerWindowID(&currentInnerWindowID);

    if (progressStateFlags & nsIWebProgressListener::STATE_START && progressStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT) {
        LOGT("currentInnerWindowID:%lu, START state:%u, status:%d", currentInnerWindowID, progressStateFlags, status);
    }
    if (progressStateFlags & nsIWebProgressListener::STATE_STOP && progressStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT) {
        LOGT("currentInnerWindowID:%lu, STOP state:%u, status:%d", currentInnerWindowID, progressStateFlags, status);
    }
    if (progressStateFlags & nsIWebProgressListener::STATE_REDIRECTING) {
        LOGT("currentInnerWindowID:%lu, REDIRECT state:%u, status:%d", currentInnerWindowID, progressStateFlags, status);
    }
    return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::OnLocationChange(nsIWebProgress* aWebProgress,
                                     nsIRequest* aRequest,
                                     nsIURI *location,
                                     uint32_t aFlags)
{
    LOGNI();
    nsCString spec;
    if (location)
        location->GetSpec(spec);
    LOGT("Location:%s", spec.get());
    return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::OnStatusChange(nsIWebProgress* aWebProgress,
                                   nsIRequest* aRequest,
                                   nsresult aStatus,
                                   const PRUnichar* aMessage)
{
    LOGNI();
    return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::OnSecurityChange(nsIWebProgress *aWebProgress,
                                     nsIRequest *aRequest,
                                     uint32_t state)
{
    LOGNI();
    return NS_OK;
}

//*****************************************************************************
// WebBrowserChrome::nsIDomEventListener
//*****************************************************************************

NS_IMETHODIMP
WebBrowserChrome::HandleEvent(nsIDOMEvent *aEvent)
{
    LOGNI();
    return NS_OK;
}

// ----- Embedding Site Window

NS_IMETHODIMP WebBrowserChrome::SetDimensions(uint32_t /*aFlags*/,
                                              int32_t /*aX*/, int32_t /*aY*/,
                                              int32_t aCx, int32_t aCy)
{
    // TODO: currently only does size
    printf("WebBrowserChrome::%s::%d sz[%i,%i]\n", __FUNCTION__, __LINE__, aCx, aCy);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetDimensions(uint32_t aFlags,
                                              int32_t* aX, int32_t* aY,
                                              int32_t* aCx, int32_t* aCy)
{
    LOGNI();
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetFocus()
{
    LOGNI();
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::GetVisibility(bool *aVisibility)
{
    LOGNI();
    *aVisibility = mIsVisible;

    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetVisibility(bool aVisibility)
{
    LOGNI();
    mIsVisible = aVisibility;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetTitle(PRUnichar ** /*aTitle*/)
{
    LOGNI();
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetTitle(const PRUnichar *aTitle)
{
    LOGNI();
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetSiteWindow(void * *aSiteWindow)
{
    NS_ENSURE_ARG_POINTER(aSiteWindow);
    printf("WebBrowserChrome::%s::%d\n", __FUNCTION__, __LINE__);
    return NS_OK;
}

/* void blur (); */
NS_IMETHODIMP WebBrowserChrome::Blur()
{
    printf("WebBrowserChrome::%s::%d\n", __FUNCTION__, __LINE__);
    return NS_ERROR_NOT_IMPLEMENTED;
}

// ----- WebBrowser Chrome Focus

NS_IMETHODIMP WebBrowserChrome::FocusNextElement()
{
    printf("WebBrowserChrome::%s::%d\n", __FUNCTION__, __LINE__);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::FocusPrevElement()
{
    printf("WebBrowserChrome::%s::%d\n", __FUNCTION__, __LINE__);
    return NS_OK;
}

void WebBrowserChrome::SetEventHandler()
{
    if (mHandlerAdded) {
        NS_ERROR("Handler was already added");
        return;
    }
    mHandlerAdded = true;
    nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebBrowser);
    NS_ENSURE_TRUE(docShell, );
    nsCOMPtr<nsIWebProgress> wp(do_GetInterface(docShell));
    NS_ENSURE_TRUE(wp, );
    nsCOMPtr<nsIWebProgressListener> listener(static_cast<nsIWebProgressListener*>(this));
    nsCOMPtr<nsIWeakReference> thisListener(do_GetWeakReference(listener));
    nsCOMPtr<nsIWebProgressListener> wpl = do_QueryReferent(thisListener);
    NS_ENSURE_TRUE(wpl, );
    wp->AddProgressListener(wpl,
                            nsIWebProgress::NOTIFY_SECURITY |
                            nsIWebProgress::NOTIFY_LOCATION |
                            nsIWebProgress::NOTIFY_STATE_NETWORK |
                            nsIWebProgress::NOTIFY_STATE_REQUEST |
                            nsIWebProgress::NOTIFY_STATE_DOCUMENT |
                            nsIWebProgress::NOTIFY_PROGRESS);

    nsCOMPtr<nsPIDOMWindow> pidomWindow = do_GetInterface(mWebBrowser);
    NS_ENSURE_TRUE(pidomWindow, );
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(pidomWindow->GetChromeEventHandler());
    NS_ENSURE_TRUE(target, );
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMContentLoaded), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMTitleChanged), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMLinkAdded), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMWillOpenModalDialog), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMModalDialogClosed), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMWindowClose), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_DOMPopupBlocked), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_pageshow), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_pagehide), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_MozScrolledAreaChanged), this,  PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING(MOZ_scroll), this,  PR_FALSE);
}

void WebBrowserChrome::RemoveEventHandler()
{
    if (!mHandlerAdded) {
        NS_ERROR("Handler was not added");
        return;
    }

    mHandlerAdded = false;
    nsCOMPtr<nsPIDOMWindow> pidomWindow = do_GetInterface(mWebBrowser);
    NS_ENSURE_TRUE(pidomWindow, );
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(pidomWindow->GetChromeEventHandler());
    NS_ENSURE_TRUE(target, );
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMContentLoaded), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMTitleChanged), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMLinkAdded), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMWillOpenModalDialog), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMModalDialogClosed), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMWindowClose), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_DOMPopupBlocked), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_pageshow), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_pagehide), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_MozScrolledAreaChanged), this,  PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING(MOZ_scroll), this,  PR_FALSE);
}
