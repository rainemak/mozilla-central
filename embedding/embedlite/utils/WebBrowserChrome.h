/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZEMBED_WEBBROWSERCHROME_H
#define MOZEMBED_WEBBROWSERCHROME_H

#include "nsCOMPtr.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsIWebProgressListener.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIDOMEventListener.h"
#include "nsIObserver.h"
#include "nsStringGlue.h"
#include "nsIObserverService.h"
#include "nsWeakReference.h"

#define kNotFound -1

class WebBrowserChrome : public nsIWebBrowserChrome,
                         public nsIWebProgressListener,
                         public nsIWebBrowserChromeFocus,
                         public nsIEmbeddingSiteWindow,
                         public nsIInterfaceRequestor,
                         public nsIObserver,
                         public nsIDOMEventListener,
                         public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIWEBBROWSERCHROMEFOCUS
    NS_DECL_NSIEMBEDDINGSITEWINDOW
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIOBSERVER
    NS_DECL_NSIDOMEVENTLISTENER

    WebBrowserChrome();

    virtual ~WebBrowserChrome();

    void SetEventHandler();
    void RemoveEventHandler();

protected:
    /* additional members */
    nsCOMPtr<nsIWebBrowser> mWebBrowser;
    uint32_t mChromeFlags;
    bool mIsModal;
    bool mIsVisible;
    bool mHandlerAdded;
};

#endif /* Header guard */

