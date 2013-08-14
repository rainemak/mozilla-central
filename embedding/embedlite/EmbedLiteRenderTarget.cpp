/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteRenderTarget"
#include "EmbedLog.h"

#include "EmbedLiteRenderTarget.h"
#include "EmbedLiteApp.h"
#include "mozilla/layers/CompositingRenderTargetOGL.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/gfx/Rect.h"

using namespace mozilla::layers;
using namespace mozilla::gfx;
using namespace mozilla::embedlite;

EmbedLiteRenderTarget::EmbedLiteRenderTarget(int width, int height, mozilla::layers::LayerManagerComposite* aComposite)
{
  SurfaceInitMode mode = INIT_MODE_CLEAR;
  IntRect rect(0, 0, width, height);
  mCurrentRenderTarget = static_cast<CompositorOGL*>(aComposite->GetCompositor())->CreateRenderTarget(rect, mode);
}

EmbedLiteRenderTarget::~EmbedLiteRenderTarget()
{
}

int
EmbedLiteRenderTarget::texture()
{
    mozilla::layers::CompositingRenderTargetOGL* ml = static_cast<mozilla::layers::CompositingRenderTargetOGL*>(mCurrentRenderTarget.get());
    return (int)ml->GetTextureHandle();
}

int
EmbedLiteRenderTarget::fbo()
{
    mozilla::layers::CompositingRenderTargetOGL* ml = static_cast<mozilla::layers::CompositingRenderTargetOGL*>(mCurrentRenderTarget.get());
    return (int)ml->GetFBO();
}
