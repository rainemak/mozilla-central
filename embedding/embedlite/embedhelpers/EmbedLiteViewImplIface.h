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
class InputData;
namespace embedlite {

class EmbedLiteViewImplIface
{
public:
    virtual void LoadURL(const char*) {}
    virtual void GoBack() {}
    virtual void GoForward() {}
    virtual void StopLoad() {}
    virtual void Reload(bool hardReload) {}
    virtual void LoadFrameScript(const char* aURI) {}
    virtual void DoSendAsyncMessage(const char* aMessageName, const char* aMessage) {}
    virtual bool RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth) { return false; }
    virtual bool RenderGL() { return false; }
    virtual void SetIsActive(bool) {}
    virtual void SetViewSize(int width, int height) {}
    virtual void SetGLViewPortSize(int width, int height) {}
    virtual void SetGLViewTransform(gfxMatrix matrix) {}
    virtual void SetTransformation(float aScale, nsIntPoint aScrollOffset) {}
    virtual void ScheduleRender() {}
    virtual void SetClipping(nsIntRect aClipRect) {}
    virtual bool ScrollBy(int aDX, int aDY, bool aDoOverflow = false) { return false; }
    virtual void ReceiveInputEvent(const InputData& aEvent) {}
    virtual void TextEvent(const char* composite, const char* preEdit) {}
    virtual void SendKeyPress(int domKeyCode, int gmodifiers, int charCode) {}
    virtual void SendKeyRelease(int domKeyCode, int gmodifiers, int charCode) {}
    virtual void MousePress(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers) {}
    virtual void MouseRelease(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers) {}
    virtual void MouseMove(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers) {}
    virtual void UpdateScrollController() {}
    virtual void ViewAPIDestroyed() {}
    virtual void UnblockPrompt(uint64_t, const bool&, const bool&, const nsString&, const nsString&, const nsString&) {}
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_VIEW_EMBED_IMPL_IFACE_H
