/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteCompositorParent"
#include "EmbedLog.h"

#include "EmbedLiteCompositorParent.h"
#include "EmbedLiteRenderTarget.h"
#include "BasicLayers.h"
#include "EmbedLiteViewThreadParent.h"
#include "EmbedLiteApp.h"
#include "EmbedLiteView.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/AsyncCompositionManager.h"
#include "mozilla/layers/LayerTransactionParent.h"
#include "mozilla/layers/CompositorOGL.h"
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
  AddRef();
  EmbedLiteView* view = EmbedLiteApp::GetInstance()->GetViewByID(mId);
  LOGT("this:%p, view:%p", this, view);
  MOZ_ASSERT(view, "Something went wrong, Compositor not suspended on destroy?");
  EmbedLiteViewThreadParent* pview = static_cast<EmbedLiteViewThreadParent*>(view->GetImpl());
  pview->SetCompositor(this);
}

EmbedLiteCompositorParent::~EmbedLiteCompositorParent()
{
  LOGT();
  EmbedLiteApp::GetInstance()->ViewDestroyed(mId);
}

PLayerTransactionParent*
EmbedLiteCompositorParent::AllocPLayerTransactionParent(const nsTArray<LayersBackend>& aBackendHints,
                                                        const uint64_t& aId,
                                                        TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                                        bool *aSuccess)
{
  EmbedLiteView* view = EmbedLiteApp::GetInstance()->GetViewByID(mId);
  EmbedLiteViewListener* listener = view ? view->GetListener() : nullptr;
  if (listener) {
    listener->CompositorCreated();
  }
  return CompositorParent::AllocPLayerTransactionParent(aBackendHints,
                                                        aId,
                                                        aTextureFactoryIdentifier,
                                                        aSuccess);
}

bool
EmbedLiteCompositorParent::IsGLBackend()
{
  return EmbedLiteApp::GetInstance()->IsAccelerated();
}

bool EmbedLiteCompositorParent::RenderToContext(gfxContext* aContext)
{
  LOGF();
  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(RootLayerTreeId());

  NS_ENSURE_TRUE(state->mLayerManager, false);
  if (!state->mLayerManager->GetRoot()) {
    // Nothing to paint yet, just return silently
    return false;
  }
#warning "Switch me to gfx::DrawTarget"
  //ComposeToTarget(aContext);
  return true;
}

bool EmbedLiteCompositorParent::RenderGL(mozilla::embedlite::EmbedLiteRenderTarget* aTarget)
{
  LOGF();
  bool retval = true;
  NS_ENSURE_TRUE(IsGLBackend(), false);

  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(RootLayerTreeId());

  if (state && IsGLBackend() && aTarget) {
    static_cast<CompositorOGL*>(state->mLayerManager->GetCompositor())->SetUserRenderTarget(aTarget->GetRenderSurface());
  }

  if (state && state->mLayerManager && state->mLayerManager->GetRoot()) {
    retval = false;
  }

  if (state && state->mLayerManager && IsGLBackend()) {
    state->mLayerManager->SetWorldTransform(mWorldTransform);
  }
  if (state && state->mLayerManager && !mActiveClipping.IsEmpty() && state->mLayerManager->GetRoot()) {
    state->mLayerManager->GetRoot()->SetClipRect(&mActiveClipping);
  }
  CompositorParent::Composite();

  if (state && IsGLBackend() && aTarget) {
    static_cast<CompositorOGL*>(state->mLayerManager->GetCompositor())->SetUserRenderTarget(nullptr);
  }

  return retval;
}

bool
EmbedLiteCompositorParent::RequestHasHWAcceleratedContext()
{
  EmbedLiteView* view = EmbedLiteApp::GetInstance()->GetViewByID(mId);
  return view ? view->GetListener()->RequestCurrentGLContext() : false;
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

void EmbedLiteCompositorParent::SetClipping(const gfxRect& aClipRect)
{
  gfxUtils::GfxRectToIntRect(aClipRect, &mActiveClipping);
}

void EmbedLiteCompositorParent::DeferredDestroyCompositor()
{
  if (GetChildCompositor()) {
    // First iteration, if child compositor available
    // Destroy it from current Child Message Loop and
    // Post task for Parent Compositor destroy in Parent MessageLoop
    NS_ASSERTION(MessageLoop::current() != EmbedLiteApp::GetInstance()->GetUILoop(),
                 "CompositorChild must be destroyed from Child Message Loop");
    GetChildCompositor()->Release();
    SetChildCompositor(nullptr, nullptr);
    EmbedLiteApp::GetInstance()->GetUILoop()->PostTask(FROM_HERE,
        NewRunnableMethod(this, &EmbedLiteCompositorParent::DeferredDestroyCompositor));
  } else {
    NS_ASSERTION(MessageLoop::current() == EmbedLiteApp::GetInstance()->GetUILoop(),
                 "CompositorParent must be destroyed from Parent Message Loop");
    // Finally destroy Parent compositor
    Release();
  }
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
  Destroy();
  // Delegate destroy of Child/Parent compositor in delayed task in order to avoid Child loop having dead objects
  mChildMessageLoop->PostTask(FROM_HERE,
                              NewRunnableMethod(this, &EmbedLiteCompositorParent::DeferredDestroyCompositor));
  return true;
}

void EmbedLiteCompositorParent::ScheduleTask(CancelableTask* task, int time)
{
  LOGF();
  EmbedLiteView* view = EmbedLiteApp::GetInstance()->GetViewByID(mId);
  if (!view) {
    LOGE("view not available.. forgot SuspendComposition call?");
    return;
  }
  if (!view->GetListener()->Invalidate()) {
    CompositorParent::ScheduleTask(task, time);
  }
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
                                            float& aScaleX, float& aScaleY,
                                            gfx::Margin& aFixedLayerMargins)
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

