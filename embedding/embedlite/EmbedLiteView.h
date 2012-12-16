/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_VIEW_EMBED_H
#define MOZ_VIEW_EMBED_H

namespace mozilla {
namespace embedlite {

class EmbedLiteViewImplIface;
class EmbedLiteView;
class EmbedLiteViewListener
{
public:
    // View intialized and ready for API calls
    virtual void ViewInitialized() {}
    // View finally destroyed and deleted
    virtual void Destroyed() {}
    virtual void LoadFinished() {}
    virtual bool Invalidate() { return false; }
};

class EmbedLiteApp;
class EmbedLiteView
{
public:
    EmbedLiteView(EmbedLiteApp* aApp);
    virtual ~EmbedLiteView();

    // Listener setup
    virtual void SetListener(EmbedLiteViewListener* aListener) { mListener = aListener; }
    virtual EmbedLiteViewListener* GetListener() { return mListener; }

    // Embed Interface
    virtual void LoadURL(const char* aUrl);
    virtual void RenderToImage(unsigned char *aData, int imgW, int imgH, int stride, int depth);

private:
    friend class EmbedLiteViewThreadParent;
    friend class EmbedLiteCompositorParent;
    void SetImpl(EmbedLiteViewImplIface*);
    EmbedLiteViewImplIface* GetImpl();

    EmbedLiteApp* mApp;
    EmbedLiteViewListener* mListener;
    EmbedLiteViewImplIface* mViewImpl;
};

} // namespace embedlite
} // namespace mozilla

#endif
