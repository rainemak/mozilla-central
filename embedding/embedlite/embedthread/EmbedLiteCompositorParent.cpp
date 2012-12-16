/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteCompositorParent"

#include "EmbedLiteCompositorParent.h"
#include "LayerManagerOGL.h"
#include "mozilla/layers/ShadowLayersParent.h"
#include "BasicLayers.h"
#include "EmbedLog.h"
#include "EmbedLiteAppThreadParent.h"
#include "EmbedLiteViewThreadParent.h"
#include "EmbedLiteApp.h"
#include "EmbedLiteView.h"

using namespace mozilla::layers;

namespace mozilla {
namespace embedlite {

EmbedLiteCompositorParent::EmbedLiteCompositorParent(nsIWidget* aWidget,
                                                     bool aRenderToEGLSurface,
                                                     int aSurfaceWidth,
                                                     int aSurfaceHeight,
                                                     uint32_t id)
  : CompositorParent(aWidget, aRenderToEGLSurface, aSurfaceWidth, aSurfaceHeight)
  , mId(id)
{
    LOGT();
    AddRef();
    EmbedLiteView* view = EmbedLiteApp::GetInstance()->GetViewByID(mId);
    EmbedLiteViewThreadParent* pview = static_cast<EmbedLiteViewThreadParent*>(view->GetImpl());
    pview->SetCompositor(this);
}

EmbedLiteCompositorParent::~EmbedLiteCompositorParent()
{
    LOGT();
}

static inline gfxASurface::gfxImageFormat
_depth_to_gfxformat(int depth)
{
    switch (depth) {
    case 32:
        return gfxASurface::ImageFormatARGB32;
    case 24:
        return gfxASurface::ImageFormatRGB24;
    case 16:
        return gfxASurface::ImageFormatRGB16_565;
    default:
        return gfxASurface::ImageFormatUnknown;
    }
}

void EmbedLiteCompositorParent::RenderToImage(unsigned char *aData,
                                              int imgW, int imgH,
                                              int stride, int depth)
{
    LOGT("data:%p, sz[%i,%i], stride:%i, depth:%i", aData, imgW, imgH, stride, depth);
    LayerManager* mgr = GetLayerManager();
    NS_ENSURE_TRUE(mgr, );
    nsRefPtr<gfxImageSurface> source =
        new gfxImageSurface(aData, gfxIntSize(imgW, imgH), stride, _depth_to_gfxformat(depth));
    {
        nsRefPtr<gfxContext> context = new gfxContext(source);
        mgr->BeginTransactionWithTarget(context);
        CompositorParent::Composite();
    }
}

void EmbedLiteCompositorParent::RenderGL()
{
    LOGT("Need GL Context init check");
}

bool EmbedLiteCompositorParent::RecvWillStop()
{
    LOGT("t");
    return CompositorParent::RecvWillStop();
}

static void DeferredDestroyCompositor(EmbedLiteCompositorParent* aCompositorParent, uint32_t id)
{
    LOGT();
    aCompositorParent->GetChildCompositor()->Release();
    aCompositorParent->Release();
}

void
EmbedLiteCompositorParent::SetChildCompositor(CompositorChild* aCompositorChild, MessageLoop* childLoop)
{
    LOGT();
    mChildMessageLoop = childLoop;
    mChildCompositor = aCompositorChild;
}

bool EmbedLiteCompositorParent::RecvStop()
{
    LOGT("t: childComp:%p, mChildMessageLoop:%p, curLoop:%p", mChildCompositor.get(), MessageLoop::current());
    mChildMessageLoop->PostTask(FROM_HERE,
               NewRunnableFunction(DeferredDestroyCompositor, this, mId));
    return CompositorParent::RecvStop();
}

bool EmbedLiteCompositorParent::RecvPause()
{
    LOGT("t");
    return CompositorParent::RecvPause();
}

bool EmbedLiteCompositorParent::RecvResume()
{
    LOGT("t");
    return CompositorParent::RecvResume();
}

bool EmbedLiteCompositorParent::RecvMakeSnapshot(const SurfaceDescriptor& aInSnapshot,
                                                 SurfaceDescriptor* aOutSnapshot)
{
    LOGT("t");
    return CompositorParent::RecvMakeSnapshot(aInSnapshot, aOutSnapshot);
}

void EmbedLiteCompositorParent::ShadowLayersUpdated(ShadowLayersParent* aLayerTree,
                                                    const TargetConfig& aTargetConfig,
                                                    bool isFirstPaint)
{
    LOGT("t");
    CompositorParent::ShadowLayersUpdated(aLayerTree,
                                          aTargetConfig,
                                          isFirstPaint);
}

void EmbedLiteCompositorParent::ScheduleComposition()
{
    LOGT("t");
    CompositorParent::ScheduleComposition();
}

PLayersParent* EmbedLiteCompositorParent::AllocPLayers(const LayersBackend& aBackendHint,
                                                       const uint64_t& aId,
                                                       LayersBackend* aBackend,
                                                       int32_t* aMaxTextureSize)
{
    LOGT("t: ALLOC PLAYERS >>>>>>>>>>>>>>>>>> ID:%llu", aId);
    return CompositorParent::AllocPLayers(aBackendHint,
                                          aId, aBackend,
                                          aMaxTextureSize);
}

bool EmbedLiteCompositorParent::DeallocPLayers(PLayersParent* aLayers)
{
    LOGT("t");
    return CompositorParent::DeallocPLayers(aLayers);
}

void EmbedLiteCompositorParent::ScheduleTask(CancelableTask* task, int time)
{
    LOGT("t");
//    if (!mView || !mView->LayersUpdated(time)) {
        CompositorParent::ScheduleTask(task, time);
//    }
}

void EmbedLiteCompositorParent::Composite()
{
    LOGT("t");
    CompositorParent::Composite();
}

void EmbedLiteCompositorParent::ComposeToTarget(gfxContext* aTarget)
{
    LOGT("t");
    CompositorParent::ComposeToTarget(aTarget);
}

void
EmbedLiteCompositorParent::SetFirstPaintViewport(const nsIntPoint& aOffset,
                                                 float aZoom, const nsIntRect& aPageRect,
                                                 const gfx::Rect& aCssPageRect)
{
    LOGT("t");
    CompositorParent::SetFirstPaintViewport(aOffset, aZoom, aPageRect, aCssPageRect);
}

void EmbedLiteCompositorParent::SetPageRect(const gfx::Rect& aCssPageRect)
{
    LOGT("t");
    CompositorParent::SetPageRect(aCssPageRect);
}

void
EmbedLiteCompositorParent::SyncViewportInfo(const nsIntRect& aDisplayPort, float aDisplayResolution,
                                            bool aLayersUpdated, nsIntPoint& aScrollOffset,
                                            float& aScaleX, float& aScaleY)
{
    LOGT("t");
    CompositorParent::SyncViewportInfo(aDisplayPort, aDisplayResolution, aLayersUpdated,
                                       aScrollOffset, aScaleX, aScaleY);
}

} // namespace embedlite
} // namespace mozilla

