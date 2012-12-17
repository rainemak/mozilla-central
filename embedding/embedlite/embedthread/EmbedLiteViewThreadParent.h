/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_VIEW_EMBED_THREAD_PARENT_H
#define MOZ_VIEW_EMBED_THREAD_PARENT_H

#include "mozilla/embedlite/PEmbedLiteViewParent.h"
#include "EmbedLiteViewImplIface.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteView;
class EmbedLiteCompositorParent;
class EmbedLiteViewThreadParent : public PEmbedLiteViewParent,
                                  public EmbedLiteViewImplIface
{
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(EmbedLiteViewThreadParent)
public:
    EmbedLiteViewThreadParent(const uint32_t& id);
    virtual ~EmbedLiteViewThreadParent();

    virtual void LoadURL(const char*);
    virtual void RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth);
    virtual void MousePress(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers);
    virtual void MouseRelease(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers);
    virtual void MouseMove(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers);
protected:
    virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
    virtual bool RecvInitialized();

private:
    friend class EmbedLiteCompositorParent;
    void SetCompositor(EmbedLiteCompositorParent* aCompositor);
    uint32_t mId;
    EmbedLiteView* mView;
    RefPtr<EmbedLiteCompositorParent> mCompositor;

    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteViewThreadParent);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_VIEW_EMBED_THREAD_PARENT_H
