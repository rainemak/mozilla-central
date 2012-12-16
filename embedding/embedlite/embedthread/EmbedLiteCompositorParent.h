/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_layers_EmbedLiteCompositorParent_h
#define mozilla_layers_EmbedLiteCompositorParent_h

#define COMPOSITOR_PERFORMANCE_WARNING

#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/CompositorChild.h"
#include "Layers.h"
#include "EmbedLiteViewThreadParent.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteCompositorParent : public mozilla::layers::CompositorParent
{
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(EmbedLiteCompositorParent)
public:
    EmbedLiteCompositorParent(nsIWidget* aWidget,
                              bool aRenderToEGLSurface,
                              int aSurfaceWidth, int aSurfaceHeight,
                              uint32_t id);
    virtual ~EmbedLiteCompositorParent();

    virtual void RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth);
    virtual void RenderGL();

    virtual bool RecvWillStop() MOZ_OVERRIDE;
    virtual bool RecvStop() MOZ_OVERRIDE;
    virtual bool RecvPause() MOZ_OVERRIDE;
    virtual bool RecvResume() MOZ_OVERRIDE;
    virtual bool RecvMakeSnapshot(const SurfaceDescriptor& aInSnapshot,
                                  SurfaceDescriptor* aOutSnapshot);
    virtual void ShadowLayersUpdated(mozilla::layers::ShadowLayersParent* aLayerTree,
                                     const mozilla::layers::TargetConfig& aTargetConfig,
                                     bool isFirstPaint) MOZ_OVERRIDE;
    virtual void ScheduleComposition();

    virtual void SetChildCompositor(mozilla::layers::CompositorChild*, MessageLoop*);
    mozilla::layers::CompositorChild* GetChildCompositor() { return mChildCompositor; }
protected:
    virtual PLayersParent* AllocPLayers(const LayersBackend& aBackendHint,
                                        const uint64_t& aId,
                                        LayersBackend* aBackend,
                                        int32_t* aMaxTextureSize);
    virtual bool DeallocPLayers(PLayersParent* aLayers);
    virtual void ScheduleTask(CancelableTask*, int);
    virtual void Composite();
    virtual void ComposeToTarget(gfxContext* aTarget);
    virtual void SetFirstPaintViewport(const nsIntPoint& aOffset, float aZoom, const nsIntRect& aPageRect, const gfx::Rect& aCssPageRect);
    virtual void SetPageRect(const gfx::Rect& aCssPageRect);
    virtual void SyncViewportInfo(const nsIntRect& aDisplayPort, float aDisplayResolution, bool aLayersUpdated,
                                  nsIntPoint& aScrollOffset, float& aScaleX, float& aScaleY);

    RefPtr<EmbedLiteViewThreadParent> mView;
    RefPtr<mozilla::layers::CompositorChild> mChildCompositor;
    MessageLoop* mChildMessageLoop;
    uint32_t mId;
};

} // embedlite
} // mozilla

#endif // mozilla_layers_EmbedLiteCompositorParent_h
