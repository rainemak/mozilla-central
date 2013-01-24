/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteCompositorParent"
#include "EmbedLog.h"

#include "EmbedLiteCompositorParent.h"
#include "LayerManagerOGL.h"
#include "mozilla/layers/ShadowLayersParent.h"
#include "BasicLayers.h"
#include "EmbedLiteAppThreadParent.h"
#include "EmbedLiteViewThreadParent.h"
#include "EmbedLiteApp.h"
#include "EmbedLiteView.h"
#include "mozilla/layers/AsyncPanZoomController.h"
#include "gfxUtils.h"

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
    EmbedLiteApp::GetInstance()->ViewDestroyed(mId);
}

bool
EmbedLiteCompositorParent::IsGLBackend()
{
    LayerManager* mgr = GetLayerManager();
    NS_ENSURE_TRUE(mgr, false);

    return mgr->GetBackendType() == mozilla::layers::LAYERS_OPENGL;
}

bool EmbedLiteCompositorParent::RenderToContext(gfxContext* aContext)
{
    LOGF();
    LayerManager* mgr = GetLayerManager();
    NS_ENSURE_TRUE(mgr, false);
    NS_ENSURE_TRUE(mgr->GetRoot(), false);
    ComposeToTarget(aContext);
    return true;
}

bool EmbedLiteCompositorParent::RenderGL()
{
    LOGF();
    bool retval = true;
    NS_ENSURE_TRUE(IsGLBackend(), false);

    LayerManager* mgr = GetLayerManager();
    if (!mgr->GetRoot()) {
        retval = false;
    }
    if (mgr->GetBackendType() == mozilla::layers::LAYERS_OPENGL) {
        static_cast<LayerManagerOGL*>(mgr)->
            SetWorldTransform(mWorldTransform);
    }
    if (!mActiveClipping.IsEmpty() && mgr->GetRoot()) {
        mgr->GetRoot()->SetClipRect(&mActiveClipping);
    }
    CompositorParent::Composite();
    return retval;
}

void EmbedLiteCompositorParent::SetSurfaceSize(int width, int height)
{
    NS_ENSURE_TRUE(IsGLBackend(),);
    CompositorParent::SetEGLSurfaceSize(width, height);
}

void EmbedLiteCompositorParent::SetWorldTransform(gfxMatrix aMatrix)
{
    mWorldTransform = aMatrix;
}

void EmbedLiteCompositorParent::SetClipping(gfxRect aClipRect)
{
    gfxUtils::GfxRectToIntRect(aClipRect, &mActiveClipping);
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

void EmbedLiteCompositorParent::ShadowLayersUpdated(ShadowLayersParent* aLayerTree,
                                                    const TargetConfig& aTargetConfig,
                                                    bool isFirstPaint)
{
    LOGF();
    CompositorParent::ShadowLayersUpdated(aLayerTree,
                                          aTargetConfig,
                                          isFirstPaint);

    Layer* shadowRoot = aLayerTree->GetRoot();
    if (ContainerLayer* root = shadowRoot->AsContainerLayer()) {
        AsyncPanZoomController* controller = GetEmbedPanZoomController();
        if (controller) {
            controller->NotifyLayersUpdated(root->GetFrameMetrics(), isFirstPaint);
        }
    }
}

void EmbedLiteCompositorParent::ScheduleTask(CancelableTask* task, int time)
{
    LOGF();
    EmbedLiteView* view = EmbedLiteApp::GetInstance()->GetViewByID(mId);
    if (!view) {
        LOGE("view not available.. forgot SuspendComposition call?");
        return;
    }
    EmbedLiteViewListener* list = view->GetListener();
    if (!list || !list->Invalidate()) {
        CompositorParent::ScheduleTask(task, time);
    }
}

AsyncPanZoomController*
EmbedLiteCompositorParent::GetEmbedPanZoomController()
{
    EmbedLiteView* view = EmbedLiteApp::GetInstance()->GetViewByID(mId);
    NS_ASSERTION(view, "View is not available");
    EmbedLiteViewThreadParent* pview = view ? static_cast<EmbedLiteViewThreadParent*>(view->GetImpl()) : nullptr;
    NS_ASSERTION(pview, "PView is not available");
    return pview ? pview->GetDefaultPanZoomController() : nullptr;
}

AsyncPanZoomController*
EmbedLiteCompositorParent::GetDefaultPanZoomController()
{
    return GetEmbedPanZoomController();
}

void
EmbedLiteCompositorParent::SetFirstPaintViewport(const nsIntPoint& aOffset,
                                                 float aZoom, const nsIntRect& aPageRect,
                                                 const gfx::Rect& aCssPageRect)
{
    LOGT("t");
    EmbedLiteView* view = EmbedLiteApp::GetInstance()->GetViewByID(mId);
    NS_ENSURE_TRUE(view, );
    view->GetListener()->SetFirstPaintViewport(aOffset, aZoom, aPageRect,
                                               gfxRect(aCssPageRect.x, aCssPageRect.y,
                                                       aCssPageRect.width, aCssPageRect.height));
}

void EmbedLiteCompositorParent::SetPageRect(const gfx::Rect& aCssPageRect)
{
    LOGT("t");
    EmbedLiteView* view = EmbedLiteApp::GetInstance()->GetViewByID(mId);
    NS_ENSURE_TRUE(view, );
    view->GetListener()->SetPageRect(gfxRect(aCssPageRect.x, aCssPageRect.y,
                                             aCssPageRect.width, aCssPageRect.height));
}

void
EmbedLiteCompositorParent::SyncViewportInfo(const nsIntRect& aDisplayPort, float aDisplayResolution,
                                            bool aLayersUpdated, nsIntPoint& aScrollOffset,
                                            float& aScaleX, float& aScaleY)
{
    LOGT("t");
    EmbedLiteView* view = EmbedLiteApp::GetInstance()->GetViewByID(mId);
    NS_ENSURE_TRUE(view, );
    view->GetListener()->SyncViewportInfo(aDisplayPort, aDisplayResolution,
                                          aLayersUpdated, aScrollOffset,
                                          aScaleX, aScaleY);
}

} // namespace embedlite
} // namespace mozilla

