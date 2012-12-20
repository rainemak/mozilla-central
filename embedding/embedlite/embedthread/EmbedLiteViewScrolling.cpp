/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteViewScrolling"

#include "EmbedLiteViewScrolling.h"
#include "EmbedLog.h"
#include "mozilla/unused.h"

using namespace mozilla::layers;

namespace mozilla {
namespace embedlite {

EmbedLiteViewScrolling::EmbedLiteViewScrolling(EmbedLiteViewThreadChild* aView)
  : mView(aView)
{
    LOGT();
}

EmbedLiteViewScrolling::~EmbedLiteViewScrolling()
{
    LOGT();
}

void
EmbedLiteViewScrolling::ViewportChange(const FrameMetrics& aMetrics, gfx::Rect cssCompositedRect)
{
    mViewport = gfx::Rect(aMetrics.mScrollOffset.x, aMetrics.mScrollOffset.y,
                          aMetrics.mViewport.width, aMetrics.mViewport.height);
    mCssCompositedRect = gfx::Rect(aMetrics.mScrollOffset.x, aMetrics.mScrollOffset.y,
                                 cssCompositedRect.width, cssCompositedRect.height);
    mCssPageRect = gfx::Rect(aMetrics.mScrollableRect.x, aMetrics.mScrollableRect.y,
                             aMetrics.mScrollableRect.width, aMetrics.mScrollableRect.height);
}

void
EmbedLiteViewScrolling::GestureDoubleTap(const nsIntPoint& aPoint)
{
}

} // namespace embedlite
} // namespace mozilla

