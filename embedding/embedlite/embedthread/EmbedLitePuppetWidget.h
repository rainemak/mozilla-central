/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=8 et :
 */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * This "puppet widget" isn't really a platform widget.  It's intended
 * to be used in widgetless rendering contexts, such as sandboxed
 * content processes.  If any "real" widgetry is needed, the request
 * is forwarded to and/or data received from elsewhere.
 */

#ifndef mozilla_widget_EmbedLitePuppetWidget_h__
#define mozilla_widget_EmbedLitePuppetWidget_h__

#include "nsBaseWidget.h"
#include "nsThreadUtils.h"
#include "nsWeakReference.h"
#include "mozilla/Attributes.h"
#include "EmbedLiteViewThreadChild.h"
#include "EmbedLog.h"

namespace mozilla {
namespace embedlite {

class EmbedLitePuppetWidget : public nsBaseWidget, public nsSupportsWeakReference
{
    typedef nsBaseWidget Base;

    // The width and height of the "widget" are clamped to this.
    static const size_t kMaxDimension;

public:
    EmbedLitePuppetWidget(EmbedLiteViewThreadChild* aEmbed, uint32_t& aId);
    virtual ~EmbedLitePuppetWidget();

    NS_DECL_ISUPPORTS_INHERITED

    NS_IMETHOD Create(nsIWidget*        aParent,
                      nsNativeWidget    aNativeParent,
                      const nsIntRect&  aRect,
                      nsDeviceContext*  aContext,
                      nsWidgetInitData* aInitData = nullptr);

    virtual already_AddRefed<nsIWidget>
    CreateChild(const nsIntRect  &aRect,
                nsDeviceContext  *aContext,
                nsWidgetInitData *aInitData = nullptr,
                bool             aForceUseIWidgetParent = false);

    NS_IMETHOD Destroy();

    NS_IMETHOD Show(bool aState);
    virtual bool IsVisible() const
        { LOGNI(); return mVisible; }
    NS_IMETHOD ConstrainPosition(bool     /*ignored aAllowSlop*/,
                                 int32_t* aX,
                                 int32_t* aY)
        { *aX = kMaxDimension;  *aY = kMaxDimension; LOGNI(); return NS_OK; }
    // We're always at <0, 0>, and so ignore move requests.
    NS_IMETHOD Move(int32_t aX, int32_t aY)
        { LOGNI(); return NS_OK; }
    NS_IMETHOD Resize(int32_t aWidth,
                      int32_t aHeight,
                      bool    aRepaint);
    NS_IMETHOD Resize(int32_t aX,
                      int32_t aY,
                      int32_t aWidth,
                      int32_t aHeight,
                      bool    aRepaint)
        // (we're always at <0, 0>)
        { LOGNI(); return Resize(aWidth, aHeight, aRepaint); }
    // XXX/cjones: copying gtk behavior here; unclear what disabling a
    // widget is supposed to entail
    NS_IMETHOD Enable(bool aState)
        { LOGNI(); mEnabled = aState;  return NS_OK; }
    virtual bool IsEnabled() const
        { LOGNI(); return mEnabled; }
    NS_IMETHOD SetFocus(bool aRaise = false);
    // PuppetWidgets don't care about children.
    virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
        { LOGNI(); return NS_OK; }
    NS_IMETHOD Invalidate(const nsIntRect& aRect);
    // PuppetWidgets don't have native data, as they're purely nonnative.
    virtual void* GetNativeData(uint32_t aDataType);
    // PuppetWidgets don't have any concept of titles..
    NS_IMETHOD SetTitle(const nsAString& aTitle)
        { LOGNI(); return NS_ERROR_UNEXPECTED; }
    // PuppetWidgets are always at <0, 0>.
    virtual nsIntPoint WidgetToScreenOffset()
        { LOGNI(); return nsIntPoint(0, 0); }
    NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus);
    NS_IMETHOD CaptureRollupEvents(nsIRollupListener* aListener,
                                   bool aDoCapture)
        { LOGNI(); return NS_ERROR_UNEXPECTED; }
    NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                      const InputContextAction& aAction);
    NS_IMETHOD_(InputContext) GetInputContext();

    // This API is going away, steer clear.
    virtual void Scroll(const nsIntPoint& aDelta,
                        const nsTArray<nsIntRect>& aDestRects,
                        const nsTArray<Configuration>& aReconfigureChildren)
        { /* dead man walking */ }
    NS_IMETHOD ReparentNativeWidget(nsIWidget* aNewParent)
        { LOGNI(); return NS_ERROR_UNEXPECTED; }

    virtual LayerManager*
    GetLayerManager(PLayersChild* aShadowManager = nullptr,
                    LayersBackend aBackendHint = mozilla::layers::LAYERS_NONE,
                    LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                    bool* aAllowRetaining = nullptr);

    virtual void CreateCompositor();
    virtual nsIntRect GetNaturalBounds();

private:
    nsresult Paint();
    void DestroyCompositor();

    EmbedLitePuppetWidget* TopWindow();
    bool IsTopLevel();

    class PaintTask : public nsRunnable {
    public:
        NS_DECL_NSIRUNNABLE
        PaintTask(EmbedLitePuppetWidget* widget) : mWidget(widget) {}
        void Revoke() { mWidget = nullptr; }
    private:
        EmbedLitePuppetWidget* mWidget;
    };

    EmbedLiteViewThreadChild* mEmbed;

    nsIntRegion mDirtyRegion;
    nsRevocableEventPtr<PaintTask> mPaintTask;

    bool mVisible;
    bool mEnabled;
    nsRefPtr<EmbedLitePuppetWidget> mChild;

    // XXX/cjones: keeping this around until we teach LayerManager to do
    // retained-content-only transactions
    nsRefPtr<gfxASurface> mSurface;
    uint32_t mId;
};

}  // namespace widget
}  // namespace mozilla

#endif  // mozilla_widget_EmbedLitePuppetWidget_h__
