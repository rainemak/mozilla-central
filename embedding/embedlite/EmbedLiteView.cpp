/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteView"

#include "EmbedLiteView.h"
#include "EmbedLiteApp.h"
#include "EmbedLog.h"

#include "mozilla/unused.h"

#include "EmbedLiteViewThreadParent.h"

namespace mozilla {
namespace embedlite {

class FakeListener : public EmbedLiteViewListener {};
EmbedLiteView::EmbedLiteView(EmbedLiteApp* aApp)
  : mApp(aApp)
  , mListener(new FakeListener())
{
    LOGT();
}

EmbedLiteView::~EmbedLiteView()
{
    LOGT("impl:%p", mViewImpl);
    if (mViewImpl && mApp->GetType() == EmbedLiteApp::EMBED_THREAD) {
        EmbedLiteViewThreadParent* impl = static_cast<EmbedLiteViewThreadParent*>(mViewImpl);
        unused << impl->SendDestroy();
    } else {
        LOGNI();
    }
    mViewImpl = NULL;
    mListener->Destroyed();
}

void
EmbedLiteView::SetImpl(EmbedLiteViewImplIface* aViewImpl)
{
    LOGT();
    mViewImpl = aViewImpl;
}

EmbedLiteViewImplIface*
EmbedLiteView::GetImpl()
{
    LOGT();
    return mViewImpl;
}

void
EmbedLiteView::LoadURL(const char* aUrl)
{
    LOGT("url:%s", aUrl);
    mViewImpl->LoadURL(aUrl);
}

void
EmbedLiteView::RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth)
{
    LOGT("data:%p, sz[%i,%i], stride:%i, depth:%i", aData, imgW, imgH, stride, depth);
    mViewImpl->RenderToImage(aData, imgW, imgH, stride, depth);
}

} // namespace embedlite
} // namespace mozilla
