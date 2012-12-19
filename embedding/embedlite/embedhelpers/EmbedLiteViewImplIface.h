/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_VIEW_EMBED_IMPL_IFACE_H
#define MOZ_VIEW_EMBED_IMPL_IFACE_H

#include "nsISupports.h"
#include "nsStringGlue.h"

#include "mozilla/RefPtr.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteViewImplIface
{
public:
    virtual void LoadURL(const char*) {}
    virtual void RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth) {}
    virtual void RenderGL() {}
    virtual void SetIsActive(bool) {}
    virtual void SetViewSize(int width, int height) {}
    virtual bool ScrollBy(int aDX, int aDY, bool aDoOverflow = false) { return false; }
    virtual void MousePress(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers) {}
    virtual void MouseRelease(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers) {}
    virtual void MouseMove(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers) {}
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_VIEW_EMBED_IMPL_IFACE_H
