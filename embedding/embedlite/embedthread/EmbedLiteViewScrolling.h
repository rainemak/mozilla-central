/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_VIEW_EMBED_SCROLLING_H
#define MOZ_VIEW_EMBED_SCROLLING_H

#include "EmbedLiteViewThreadChild.h"
#include "Layers.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteViewScrolling
{
    NS_INLINE_DECL_REFCOUNTING(EmbedLiteViewScrolling)
public:
    EmbedLiteViewScrolling(EmbedLiteViewThreadChild* aView);
    virtual ~EmbedLiteViewScrolling();

    void ViewportChange(const mozilla::layers::FrameMetrics& aMetrics, gfx::Rect cssCompositedRect);
    void GestureDoubleTap(const nsIntPoint& aPoint);
    void ScrollToFocusedInput(bool aAllowZoom);
    void AsyncScrollDOMEvent(const gfxRect& contentRect, const gfxSize& scrollSize);

private:
    bool IsRectZoomedIn(gfx::Rect aRect, gfx::Rect aViewport);
    bool ShouldZoomToElement(nsIDOMElement* aElement);
    void AnyElementFromPoint(nsIDOMWindow* aWindow, double aX, double aY, nsIDOMElement* *aElem);
    gfx::Rect GetBoundingContentRect(nsIDOMElement* aElement);
    nsresult GetFocusedInput(nsIDOMElement* *aElement, bool aOnlyInputElements = false);
    void ZoomToElement(nsIDOMElement* aElement,
                       int aClickY = -1,
                       bool aCanZoomOut = true,
                       bool aCanZoomIn = true);

    gfx::Rect mViewport;
    gfx::Rect mCssCompositedRect;
    gfx::Rect mCssPageRect;
    gfxRect mContentRect;
    gfxSize mScrollSize;

    EmbedLiteViewThreadChild* mView;
    bool mGotViewPortUpdate;
    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteViewScrolling);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_VIEW_EMBED_SCROLLING_H
