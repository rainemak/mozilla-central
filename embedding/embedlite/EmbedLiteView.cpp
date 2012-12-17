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
#include "EmbedKineticModule.h"

namespace mozilla {
namespace embedlite {

class FakeListener : public EmbedLiteViewListener {};
class MyKineticListener : public EmbedKineticListener
{
public:
    MyKineticListener(EmbedLiteView* aView) : mView(aView) {}
    virtual void ScrollViewBy(int dx, int dy)
    {
        LOGT("pt[%i,%i]", dx, dy);
        mView->ScrollBy(dx, dy);
    }
    virtual void UpdateViewport()
    {
        LOGT();
    }
private:
    EmbedLiteView* mView;
};

EmbedLiteView::EmbedLiteView(EmbedLiteApp* aApp)
  : mApp(aApp)
  , mListener(new FakeListener())
  , mScrollingMode(false)
  , mKineticListener(new MyKineticListener(this))
  , mKinetic(new EmbedKineticModule(mKineticListener))
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
    mKinetic = nullptr;
    mKineticListener = nullptr;
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

bool
EmbedLiteView::ScrollBy(int aDX, int aDY, bool aDoOverflow)
{
    LOGT();
    return mViewImpl->ScrollBy(aDX, aDY);
}

void
EmbedLiteView::MousePress(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    mViewImpl->MousePress(x, y, mstime, buttons, modifiers);
    mKinetic->MousePress(x, y, mstime);
}

void
EmbedLiteView::MouseRelease(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    mViewImpl->MouseRelease(x, y, mstime, buttons, modifiers);
    mKinetic->MouseRelease(x, y, mstime);
}

void
EmbedLiteView::MouseMove(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    mViewImpl->MouseMove(x, y, mstime, buttons, modifiers);
    mKinetic->MouseMove(x, y, mstime);
}

} // namespace embedlite
} // namespace mozilla
