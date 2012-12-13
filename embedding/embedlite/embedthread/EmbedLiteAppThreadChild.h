/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_APP_EMBED_THREAD_CHILD_H
#define MOZ_APP_EMBED_THREAD_CHILD_H

#include "mozilla/embedlite/PEmbedLiteAppChild.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteAppThreadChild : public PEmbedLiteAppChild
{
    NS_INLINE_DECL_REFCOUNTING(EmbedLiteAppThreadChild)
public:
    EmbedLiteAppThreadChild();
    virtual ~EmbedLiteAppThreadChild();

protected:
    // Embed API ipdl interface
    virtual bool RecvSetBoolPref(const nsCString&, const bool&);
    virtual bool RecvSetCharPref(const nsCString&, const nsCString&);
    virtual bool RecvSetIntPref(const nsCString&, const int&);

    // IPDL protocol impl
    virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

private:
    void InitWindowWatcher();

    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteAppThreadChild);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_APP_EMBED_THREAD_CHILD_H
