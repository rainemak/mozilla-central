/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_VIEW_EMBED_THREAD_CHILD_H
#define MOZ_VIEW_EMBED_THREAD_CHILD_H

#include "mozilla/embedlite/PEmbedLiteViewChild.h"

#include "nsIWebBrowser.h"
#include "nsIWidget.h"
#include "nsIWebNavigation.h"
#include "WebBrowserChrome.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteViewThreadChild : public PEmbedLiteViewChild
{
    NS_INLINE_DECL_REFCOUNTING(EmbedLiteViewThreadChild)
public:
    EmbedLiteViewThreadChild(uint32_t);
    virtual ~EmbedLiteViewThreadChild();

protected:
    virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
    virtual bool RecvDestroy();
    virtual bool RecvLoadURL(const nsString&);

private:
    void InitGeckoWindow();

    uint32_t mId;
    nsCOMPtr<nsIWidget> mWidget;
    nsCOMPtr<nsIWebBrowser> mWebBrowser;
    nsCOMPtr<nsIWebBrowserChrome> mChrome;
    nsCOMPtr<nsIDOMWindow> mDOMWindow;
    nsCOMPtr<nsIWebNavigation> mWebNavigation;
    WebBrowserChrome* mBChrome;

    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteViewThreadChild);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_VIEW_EMBED_THREAD_CHILD_H
