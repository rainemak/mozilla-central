/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EmbedChildListener_H_
#define EmbedChildListener_H_

#include "nsWeakReference.h"
#include "nsIObserver.h"

class EmbedChildListener : public nsIObserver,
                           public nsSupportsWeakReference
{
public:
    EmbedChildListener();
    virtual ~EmbedChildListener();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsresult Init();
};

#define NS_EMBED_CHILD_CONTRACTID "@mozilla.org/embed-child-starter;1"
#define NS_EMBED_CHILD_SERVICE_CLASSNAME "EmbedChild Component"
#define NS_EMBED_CHILD_SERVICE_CID \
{ 0xa1552da1, \
  0x8122, \
  0x48ad, \
  { 0xa5, 0x9f, 0x4f, 0xcc, 0xae, 0x6e, 0x8e, 0xc6 }}

#endif /*EmbedChildListener_H_*/
