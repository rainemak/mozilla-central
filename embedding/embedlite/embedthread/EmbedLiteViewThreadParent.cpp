/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteViewThreadParent"

#include "EmbedLiteViewThreadParent.h"
#include "EmbedLog.h"
#include "EmbedLiteApp.h"
#include "EmbedLiteView.h"

#include "EmbedLiteCompositorParent.h"
#include "mozilla/unused.h"

namespace mozilla {
namespace embedlite {

EmbedLiteViewThreadParent::EmbedLiteViewThreadParent(const uint32_t& id)
  : mId(id)
  , mView(EmbedLiteApp::GetInstance()->GetViewByID(id))
  , mCompositor(nullptr)
{
    MOZ_COUNT_CTOR(EmbedLiteViewThreadParent);
    LOGT("id:%u", mId);
    mView->SetImpl(this);
}

EmbedLiteViewThreadParent::~EmbedLiteViewThreadParent()
{
    MOZ_COUNT_DTOR(EmbedLiteViewThreadParent);
    LOGT();
    mView->SetImpl(NULL);
}

void
EmbedLiteViewThreadParent::ActorDestroy(ActorDestroyReason aWhy)
{
    LOGT("reason:%i", aWhy);
}

bool
EmbedLiteViewThreadParent::RecvInitialized()
{
    LOGT();
    mView->GetListener()->ViewInitialized();
    return true;
}

void
EmbedLiteViewThreadParent::SetCompositor(EmbedLiteCompositorParent* aCompositor)
{
    LOGT();
    mCompositor = aCompositor;
}

void
EmbedLiteViewThreadParent::LoadURL(const char* aUrl)
{
    LOGT();
    unused << SendLoadURL(NS_ConvertUTF8toUTF16(nsCString(aUrl)));
}

void
EmbedLiteViewThreadParent::RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth)
{
    LOGT();
    if (mCompositor) {
        mCompositor->RenderToImage(aData, imgW, imgH, stride, depth);
    }
}

void
EmbedLiteViewThreadParent::MousePress(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    LOGT("pt[%i,%i], t:%i, bt:%u, mod:%u", x, y, mstime, buttons, modifiers);
}

void
EmbedLiteViewThreadParent::MouseRelease(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    LOGT("pt[%i,%i], t:%i, bt:%u, mod:%u", x, y, mstime, buttons, modifiers);
}

void
EmbedLiteViewThreadParent::MouseMove(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    LOGT("pt[%i,%i], t:%i, bt:%u, mod:%u", x, y, mstime, buttons, modifiers);
}

} // namespace embedlite
} // namespace mozilla
