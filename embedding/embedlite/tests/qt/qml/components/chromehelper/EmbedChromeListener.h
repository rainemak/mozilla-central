/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EmbedChromeListener_H_
#define EmbedChromeListener_H_

#include "nsWeakReference.h"
#include "nsIObserver.h"
#include "nsIDOMEventListener.h"

class EmbedChromeListener : public nsIObserver,
                            public nsIDOMEventListener,
                            public nsSupportsWeakReference
{
public:
    EmbedChromeListener();
    virtual ~EmbedChromeListener();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSIDOMEVENTLISTENER

    nsresult Init();
};

#define NS_EMBED_CHROME_CONTRACTID "@mozilla.org/embed-chrome-component;1"
#define NS_EMBED_CHROME_SERVICE_CLASSNAME "Embed Chrome Listener Component"
#define NS_EMBED_CHROME_SERVICE_CID \
{ 0xab157a4c, \
  0x6aaf, \
  0x12e2, \
  { 0xa6, 0x9f, 0x4f, 0xcc, 0xae, 0x4e, 0x8e, 0xc6 }}

#endif /*EmbedChromeListener_H_*/
