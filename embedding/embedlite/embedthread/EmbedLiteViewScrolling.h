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

private:
    gfx::Rect mViewport;
    gfx::Rect mCssCompositedRect;
    gfx::Rect mCssPageRect;

    EmbedLiteViewThreadChild* mView;
    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteViewScrolling);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_VIEW_EMBED_SCROLLING_H
