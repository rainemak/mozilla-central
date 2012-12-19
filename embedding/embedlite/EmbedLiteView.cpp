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

// Image as URL includes
#include "gfxImageSurface.h"
#include "mozilla/Base64.h"
#include "imgIEncoder.h"

namespace mozilla {
namespace embedlite {

class FakeListener : public EmbedLiteViewListener {};

EmbedLiteView::EmbedLiteView(EmbedLiteApp* aApp)
  : mApp(aApp)
  , mListener(new FakeListener())
  , mViewImpl(NULL)
  , mScrollingMode(false)
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
    NS_ENSURE_TRUE(mViewImpl, );
    mViewImpl->LoadURL(aUrl);
}

void
EmbedLiteView::RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth)
{
    LOGT("data:%p, sz[%i,%i], stride:%i, depth:%i", aData, imgW, imgH, stride, depth);
    NS_ENSURE_TRUE(mViewImpl, );
    mViewImpl->RenderToImage(aData, imgW, imgH, stride, depth);
}

char*
EmbedLiteView::GetImageAsURL(int aWidth, int aHeight)
{
    // copy from gfxASurface::WriteAsPNG_internal
    NS_ENSURE_TRUE(mViewImpl, nullptr);
    nsRefPtr<gfxImageSurface> img =
        new gfxImageSurface(gfxIntSize(aWidth, aHeight), gfxASurface::ImageFormatRGB24);
    mViewImpl->RenderToImage(img->Data(), img->Width(), img->Height(), img->Stride(), 24);
    nsCOMPtr<imgIEncoder> encoder =
        do_CreateInstance("@mozilla.org/image/encoder;2?type=image/png");
    NS_ENSURE_TRUE(encoder, nullptr);
    gfxIntSize size = img->GetSize();
    nsresult rv = encoder->InitFromData(img->Data(),
                                        size.width * size.height * 4,
                                        size.width,
                                        size.height,
                                        img->Stride(),
                                        imgIEncoder::INPUT_FORMAT_HOSTARGB,
                                        NS_LITERAL_STRING(""));
    if (NS_FAILED(rv))
        return nullptr;
    nsCOMPtr<nsIInputStream> imgStream;
    CallQueryInterface(encoder.get(), getter_AddRefs(imgStream));

    if (!imgStream)
        return nullptr;

    uint64_t bufSize64;
    rv = imgStream->Available(&bufSize64);
    if (NS_FAILED(rv))
        return nullptr;

    if (bufSize64 > UINT32_MAX - 16)
        return nullptr;

    uint32_t bufSize = (uint32_t)bufSize64;

    // ...leave a little extra room so we can call read again and make sure we
    // got everything. 16 bytes for better padding (maybe)
    bufSize += 16;
    uint32_t imgSize = 0;
    char* imgData = (char*)moz_malloc(bufSize);
    if (!imgData)
        return nullptr;
    uint32_t numReadThisTime = 0;
    while ((rv = imgStream->Read(&imgData[imgSize],
                                 bufSize - imgSize,
                                 &numReadThisTime)) == NS_OK && numReadThisTime > 0)
    {
        imgSize += numReadThisTime;
        if (imgSize == bufSize) {
            // need a bigger buffer, just double
            bufSize *= 2;
            char* newImgData = (char*)moz_realloc(imgData, bufSize);
            if (!newImgData) {
                moz_free(imgData);
                return nullptr;
            }
            imgData = newImgData;
        }
    }

    // base 64, result will be NULL terminated
    nsCString encodedImg;
    rv = Base64Encode(Substring(imgData, imgSize), encodedImg);
    moz_free(imgData);
    if (NS_FAILED(rv)) // not sure why this would fail
        return nullptr;

    nsCString string("data:image/png;base64,");
    string.Append(encodedImg);

    return ToNewCString(string);
}

void
EmbedLiteView::SetViewSize(int width, int height)
{
    LOGNI("sz[%i,%i]", width, height);
    NS_ENSURE_TRUE(mViewImpl, );
    mViewImpl->SetViewSize(width, height);
}

bool
EmbedLiteView::ScrollBy(int aDX, int aDY, bool aDoOverflow)
{
    LOGT();
    NS_ENSURE_TRUE(mViewImpl, false);
    return mViewImpl->ScrollBy(aDX, aDY);
}

void
EmbedLiteView::MousePress(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    NS_ENSURE_TRUE(mViewImpl, );
    mViewImpl->MousePress(x, y, mstime, buttons, modifiers);
}

void
EmbedLiteView::MouseRelease(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    NS_ENSURE_TRUE(mViewImpl, );
    mViewImpl->MouseRelease(x, y, mstime, buttons, modifiers);
}

void
EmbedLiteView::MouseMove(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers)
{
    NS_ENSURE_TRUE(mViewImpl, );
    mViewImpl->MouseMove(x, y, mstime, buttons, modifiers);
}

} // namespace embedlite
} // namespace mozilla
