/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "WindowCreator"
#include "EmbedLog.h"

#include "nsIWebBrowserChrome.h"
#include "WindowCreator.h"
#include <stdio.h>

WindowCreator::WindowCreator()
  : mOpenBlock(false)
{
    LOGT();
}

WindowCreator::~WindowCreator()
{
    LOGT();
}

NS_IMPL_ISUPPORTS2(WindowCreator, nsIWindowCreator, nsIWindowCreator2)

NS_IMETHODIMP
WindowCreator::CreateChromeWindow2(nsIWebBrowserChrome* aParent,
                                   uint32_t aChromeFlags,
                                   uint32_t aContextFlags,
                                   nsIURI* aURI,
                                   bool* aCancel,
                                   nsIWebBrowserChrome* *_retval)
{
    NS_ENSURE_ARG_POINTER(aCancel);
    NS_ENSURE_ARG_POINTER(_retval);
    *aCancel = false;
    *_retval = 0;
  
    LOGNI();
/*
    See bug 80707
    Desktop FF allow to create popup window if aChromeFlags == 1670, aContextFlags == 0
*/

    if (mOpenBlock) {
        mOpenBlock = PR_FALSE;
        return NS_ERROR_FAILURE;
    }
/*
    // No parent?  Ask via the singleton object instead.
    *_retval = static_cast<nsIWebBrowserChrome*>(mContext->RequestNewWindow(aParent, aChromeFlags));
    if (*_retval) {
        NS_ADDREF(*_retval);
        return NS_OK;
    }
*/
    *_retval = nullptr;
    // check to make sure that we made a new window
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
WindowCreator::CreateChromeWindow(nsIWebBrowserChrome* aParent,
                                  uint32_t aChromeFlags,
                                  nsIWebBrowserChrome* *_retval)
{
    LOGNI();
    bool cancel;
    return CreateChromeWindow2(aParent, aChromeFlags, 0, 0, &cancel, _retval);
}
