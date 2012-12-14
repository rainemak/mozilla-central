/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_VIEW_EMBED_THREAD_PARENT_H
#define MOZ_VIEW_EMBED_THREAD_PARENT_H

#include "mozilla/embedlite/PEmbedLiteViewParent.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteViewThreadParent : public PEmbedLiteViewParent
{
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(EmbedLiteViewThreadParent)
public:
    EmbedLiteViewThreadParent(const uint32_t& id);
    virtual ~EmbedLiteViewThreadParent();

protected:
    virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

private:
    uint32_t mId;

    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteViewThreadParent);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_VIEW_EMBED_THREAD_PARENT_H
