/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=8 et :
 */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLitePuppetWidget"

#include "base/basictypes.h"

#include "BasicLayers.h"
#include "gfxPlatform.h"
#if defined(MOZ_ENABLE_D3D10_LAYER)
# include "LayerManagerD3D10.h"
#endif
#include "mozilla/Hal.h"
#include "mozilla/layers/CompositorChild.h"
#include "mozilla/layers/PLayersChild.h"
#include "EmbedLitePuppetWidget.h"
#include "nsIWidgetListener.h"

#include "Layers.h"
#include "BasicLayers.h"
#include "LayerManagerOGL.h"
#include "GLContext.h"
#include "GLContextProvider.h"
#include "EmbedLiteCompositorParent.h"

using namespace mozilla::dom;
using namespace mozilla::hal;
using namespace mozilla::layers;
using namespace mozilla::widget;
using mozilla::ipc::AsyncChannel;

namespace mozilla {
namespace embedlite {

// Arbitrary, fungible.
const size_t EmbedLitePuppetWidget::kMaxDimension = 4000;

static nsTArray<EmbedLitePuppetWidget*> gTopLevelWindows;
static bool sFailedToCreateGLContext = false;
static nsRefPtr<gl::GLContext> sGLContext;
static bool sValidSurface = false;

NS_IMPL_ISUPPORTS_INHERITED1(EmbedLitePuppetWidget, nsBaseWidget,
                             nsISupportsWeakReference)

static bool
IsPopup(const nsWidgetInitData* aInitData)
{
    return aInitData && aInitData->mWindowType == eWindowType_popup;
}

static void
InvalidateRegion(nsIWidget* aWidget, const nsIntRegion& aRegion)
{
    nsIntRegionRectIterator it(aRegion);
    while(const nsIntRect* r = it.Next()) {
        aWidget->Invalidate(*r);
    }
}

EmbedLitePuppetWidget*
EmbedLitePuppetWidget::TopWindow()
{
    if (!gTopLevelWindows.IsEmpty())
        return gTopLevelWindows[0];
    return nullptr;
}

bool
EmbedLitePuppetWidget::IsTopLevel()
{
    return mWindowType == eWindowType_toplevel ||
           mWindowType == eWindowType_dialog ||
           mWindowType == eWindowType_invisible;
}

void EmbedLitePuppetWidget::DestroyCompositor() 
{
    if (mCompositorChild) {
        mCompositorChild->SendWillStop();

        // The call just made to SendWillStop can result in IPC from the
        // CompositorParent to the CompositorChild (e.g. caused by the destruction
        // of shared memory). We need to ensure this gets processed by the
        // CompositorChild before it gets destroyed. It suffices to ensure that
        // events already in the MessageLoop get processed before the
        // CompositorChild is destroyed, so we add a task to the MessageLoop to
        // handle compositor desctruction.
        MessageLoop::current()->PostTask(FROM_HERE,
                                         NewRunnableMethod(mCompositorChild.get(), &CompositorChild::Destroy));
        // The DestroyCompositor task we just added to the MessageLoop will handle
        // releasing mCompositorParent and mCompositorChild.
        mCompositorParent.forget();
        mCompositorChild.forget();
    }
}

EmbedLitePuppetWidget::EmbedLitePuppetWidget(EmbedLiteViewThreadChild* aEmbed, uint32_t& aId)
  : mEmbed(aEmbed)
  , mVisible(false)
  , mEnabled(false)
  , mId(aId)
{
    MOZ_COUNT_CTOR(EmbedLitePuppetWidget);
    LOGT();
}

EmbedLitePuppetWidget::~EmbedLitePuppetWidget()
{
    MOZ_COUNT_DTOR(EmbedLitePuppetWidget);
    LOGT();
    gTopLevelWindows.RemoveElement(this);
    DestroyCompositor();
}

NS_IMETHODIMP
EmbedLitePuppetWidget::Create(nsIWidget        *aParent,
                              nsNativeWidget   aNativeParent,
                              const nsIntRect  &aRect,
                              nsDeviceContext *aContext,
                              nsWidgetInitData *aInitData)
{
    LOGT();
    NS_ABORT_IF_FALSE(!aNativeParent, "got a non-Puppet native parent");

    BaseCreate(nullptr, aRect, aContext, aInitData);

    mBounds = aRect;
    mEnabled = true;
    mVisible = true;

    mSurface = gfxPlatform::GetPlatform()
               ->CreateOffscreenSurface(gfxIntSize(1, 1),
                                        gfxASurface::ContentFromFormat(gfxASurface::ImageFormatRGB24));

    EmbedLitePuppetWidget* parent = static_cast<EmbedLitePuppetWidget*>(aParent);
    if (parent) {
        parent->mChild = this;
        mLayerManager = parent->GetLayerManager();
    }
    else {
        Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, false);
    }

    if (IsTopLevel()) {
        LOGT("Append this to toplevel windows:%p", this);
        gTopLevelWindows.AppendElement(this);
    }

    return NS_OK;
}

already_AddRefed<nsIWidget>
EmbedLitePuppetWidget::CreateChild(const nsIntRect  &aRect,
                                   nsDeviceContext  *aContext,
                                   nsWidgetInitData *aInitData,
                                   bool              aForceUseIWidgetParent)
{
    LOGT();
    bool isPopup = IsPopup(aInitData);
    nsCOMPtr<nsIWidget> widget = new EmbedLitePuppetWidget(mEmbed, mId);
    return ((widget &&
             NS_SUCCEEDED(widget->Create(isPopup ? nullptr: this, nullptr, aRect,
                                         aContext, aInitData))) ?
             widget.forget() : nullptr);
}

NS_IMETHODIMP
EmbedLitePuppetWidget::Destroy()
{
    LOGT();
    Base::OnDestroy();
    Base::Destroy();
    mPaintTask.Revoke();
    mChild = nullptr;
    if (mLayerManager) {
        mLayerManager->Destroy();
    }
    mLayerManager = nullptr;
    mEmbed = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
EmbedLitePuppetWidget::Show(bool aState)
{
    LOGT();
    NS_ASSERTION(mEnabled,
                 "does it make sense to Show()/Hide() a disabled widget?");

    bool wasVisible = mVisible;
    mVisible = aState;

    if (mChild) {
        mChild->mVisible = aState;
    }

    if (!mVisible && mLayerManager) {
        mLayerManager->ClearCachedResources();
    }

    if (!wasVisible && mVisible) {
        Resize(mBounds.width, mBounds.height, false);
        Invalidate(mBounds);
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedLitePuppetWidget::Resize(double aWidth,
                              double aHeight,
                              bool    aRepaint)
{
    LOGT();
    nsIntRect oldBounds = mBounds;
    mBounds.SizeTo(nsIntSize(NSToIntRound(aWidth), NSToIntRound(aHeight)));

    if (mChild) {
        return mChild->Resize(aWidth, aHeight, aRepaint);
    }

    // XXX: roc says that |aRepaint| dictates whether or not to
    // invalidate the expanded area
    if (oldBounds.Size() < mBounds.Size() && aRepaint) {
        nsIntRegion dirty(mBounds);
        dirty.Sub(dirty,  oldBounds);
        InvalidateRegion(this, dirty);
    }

    nsIWidgetListener *listener =
        mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;
    if (!oldBounds.IsEqualEdges(mBounds) && listener) {
        listener->WindowResized(this, mBounds.width, mBounds.height);
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedLitePuppetWidget::SetFocus(bool aRaise)
{
    LOGNI();
    return NS_OK;
}

NS_IMETHODIMP
EmbedLitePuppetWidget::Invalidate(const nsIntRect& aRect)
{
    if (mChild) {
        return mChild->Invalidate(aRect);
    }

    mDirtyRegion.Or(mDirtyRegion, aRect);

    if (!mDirtyRegion.IsEmpty() && !mPaintTask.IsPending()) {
        mPaintTask = new PaintTask(this);
        return NS_DispatchToCurrentThread(mPaintTask.get());
    }

    return NS_OK;
}

void*
EmbedLitePuppetWidget::GetNativeData(uint32_t aDataType)
{
    LOGT("DataType: %i", aDataType);
    switch (aDataType) {
    case NS_NATIVE_SHAREABLE_WINDOW: {
        LOGW("aDataType:%i\n", __LINE__, aDataType);
        return (void*)nullptr;
    }
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_DISPLAY:
    case NS_NATIVE_PLUGIN_PORT:
    case NS_NATIVE_GRAPHIC:
    case NS_NATIVE_SHELLWIDGET:
    case NS_NATIVE_WIDGET:
        LOGW("nsWindow::GetNativeData not implemented for this type");
        break;
    default:
        NS_WARNING("nsWindow::GetNativeData called with bad value");
        break;
    }

    return nullptr;
}

NS_IMETHODIMP
EmbedLitePuppetWidget::DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus)
{
    NS_ABORT_IF_FALSE(!mChild || mChild->mWindowType == eWindowType_popup,
                      "Unexpected event dispatch!");

    aStatus = nsEventStatus_eIgnore;

    nsIWidgetListener *listener =
        mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;

    NS_ABORT_IF_FALSE(listener, "No listener!");

#if 0
    if (event->message == NS_COMPOSITION_START) {
        mIMEComposing = true;
    }
    switch (event->eventStructType) {
    case NS_COMPOSITION_EVENT:
        mIMELastReceivedSeqno = static_cast<nsCompositionEvent*>(event)->seqno;
        if (mIMELastReceivedSeqno < mIMELastBlurSeqno)
            return NS_OK;
        break;
    case NS_TEXT_EVENT:
        mIMELastReceivedSeqno = static_cast<nsTextEvent*>(event)->seqno;
        if (mIMELastReceivedSeqno < mIMELastBlurSeqno)
            return NS_OK;
        break;
    case NS_SELECTION_EVENT:
        mIMELastReceivedSeqno = static_cast<nsSelectionEvent*>(event)->seqno;
        if (mIMELastReceivedSeqno < mIMELastBlurSeqno)
            return NS_OK;
        break;
    default:
        break;
    }
#endif

    aStatus = listener->HandleEvent(event, mUseAttachedEvents);

#if 0
    if (event->message == NS_COMPOSITION_END) {
        mIMEComposing = false;
    }
#endif

    return NS_OK;
}

NS_IMETHODIMP_(void)
EmbedLitePuppetWidget::SetInputContext(const InputContext& aContext,
                                       const InputContextAction& aAction)
{
    LOGNI();
}

NS_IMETHODIMP_(InputContext)
EmbedLitePuppetWidget::GetInputContext()
{
    LOGNI();
    InputContext context;
    return context;
}

LayerManager*
EmbedLitePuppetWidget::GetLayerManager(PLayersChild* aShadowManager,
                                       LayersBackend aBackendHint,
                                       LayerManagerPersistence aPersistence,
                                       bool* aAllowRetaining)
{
    LOGT();
    if (aAllowRetaining)
        *aAllowRetaining = true;

    if (mLayerManager) {
        return mLayerManager;
    }

    EmbedLitePuppetWidget *topWindow = TopWindow();

    if (!topWindow) {
        printf_stderr(" -- no topwindow\n");
        mLayerManager = CreateBasicLayerManager();
        return mLayerManager;
    }

    mUseLayersAcceleration = ComputeShouldAccelerate(mUseLayersAcceleration);

    bool useCompositor = UseOffMainThreadCompositing();

    if (useCompositor) {
        CreateCompositor();
        if (mLayerManager) {
            return mLayerManager;
        }

        // If we get here, then off main thread compositing failed to initialize.
        sFailedToCreateGLContext = true;
    }

    if (!mUseLayersAcceleration ||
        sFailedToCreateGLContext)
    {
        printf_stderr(" -- creating basic, not accelerated\n");
        mLayerManager = CreateBasicLayerManager();
        return mLayerManager;
    }

    if (!mLayerManager) {
        if (!sGLContext) {
            // the window we give doesn't matter here
            sGLContext = mozilla::gl::GLContextProvider::CreateForWindow(this);
        }

        if (sGLContext) {
            nsRefPtr<mozilla::layers::LayerManagerOGL> layerManager =
                new mozilla::layers::LayerManagerOGL(this);

            if (layerManager && layerManager->Initialize(sGLContext))
                mLayerManager = layerManager;
            sValidSurface = true;
        }

        if (!sGLContext || !mLayerManager) {
            sGLContext = nullptr;
            sFailedToCreateGLContext = true;
            mLayerManager = CreateBasicLayerManager();
        }
    }

    return mLayerManager;
}

void EmbedLitePuppetWidget::CreateCompositor()
{
    LOGT();
    bool renderToEGLSurface = true;
    nsIntRect rect;
    GetBounds(rect);
    EmbedLiteCompositorParent* parent = new EmbedLiteCompositorParent(this, renderToEGLSurface, rect.width, rect.height, mId);
    mCompositorParent = parent;
    LayerManager* lm = CreateBasicLayerManager();
    MessageLoop *childMessageLoop = CompositorParent::CompositorLoop();
    mCompositorChild = new CompositorChild(lm);
    parent->SetChildCompositor(mCompositorChild, MessageLoop::current());
    AsyncChannel *parentChannel = mCompositorParent->GetIPCChannel();
    AsyncChannel::Side childSide = mozilla::ipc::AsyncChannel::Child;
    mCompositorChild->Open(parentChannel, childMessageLoop, childSide);
    int32_t maxTextureSize;
    PLayersChild* shadowManager;
    mozilla::layers::LayersBackend backendHint =
        mUseLayersAcceleration ? mozilla::layers::LAYERS_OPENGL : mozilla::layers::LAYERS_BASIC;
    mozilla::layers::LayersBackend parentBackend;
    shadowManager = mCompositorChild->SendPLayersConstructor(
        backendHint, 0, &parentBackend, &maxTextureSize);

    if (shadowManager) {
        ShadowLayerForwarder* lf = lm->AsShadowForwarder();
        if (!lf) {
            delete lm;
            mCompositorChild = nullptr;
            return;
        }
        lf->SetShadowManager(shadowManager);
        lf->SetParentBackendType(parentBackend);
        lf->SetMaxTextureSize(maxTextureSize);

        mLayerManager = lm;
    } else {
        // We don't currently want to support not having a LayersChild
        NS_RUNTIMEABORT("failed to construct LayersChild");
        delete lm;
        mCompositorChild = nullptr;
    }
}

nsresult
EmbedLitePuppetWidget::Paint()
{
    NS_ABORT_IF_FALSE(!mDirtyRegion.IsEmpty(), "paint event logic messed up");

    nsIWidgetListener *listener =
        mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;

    if (!listener)
        return NS_OK;

    // Dispatch WillPaintWindow notification to allow scripts etc. to run
    // before we paint
    {
        listener->WillPaintWindow(this, true);

        // If the window has been destroyed during the will paint notification,
        // there is nothing left to do.
        if (!mEmbed)
            return NS_OK;

        // Re-get the listener since the will paint notification might have
        // killed it.
        listener =
            mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;
        if (!listener)
            return NS_OK;
    }

    nsIntRegion region = mDirtyRegion;

    // reset repaint tracking
    mDirtyRegion.SetEmpty();
    mPaintTask.Revoke();

    nsRefPtr<EmbedLitePuppetWidget> Hold(this);

    {
#ifdef DEBUG
        debug_DumpPaintEvent(stderr, this, region,
                             nsAutoCString("MozEmbedWidget"), 0);
#endif
        // FIXME, we should be able to initiate forward paint transaction
        // without fake surface
        nsRefPtr<gfxContext> ctx = new gfxContext(mSurface);

        ctx->Rectangle(gfxRect(0,0,0,0));
        ctx->Clip();
        AutoLayerManagerSetup setupLayerManager(this, ctx,
                                                mozilla::layers::BUFFER_NONE);
        listener->PaintWindow(this, region, nsIWidgetListener::WILL_SEND_DID_PAINT | nsIWidgetListener::SENT_WILL_PAINT);
    }

    listener = mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;
    if (!listener)
        return NS_OK;

    listener->DidPaintWindow();

    return NS_OK;
}

NS_IMETHODIMP
EmbedLitePuppetWidget::PaintTask::Run()
{
    if (mWidget) {
        mWidget->Paint();
    }
    return NS_OK;
}

nsIntRect
EmbedLitePuppetWidget::GetNaturalBounds()
{
    return nsIntRect();
}

bool
EmbedLitePuppetWidget::HasGLContext()
{
    return true;
}

}  // namespace widget
}  // namespace mozilla
