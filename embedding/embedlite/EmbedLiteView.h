/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_VIEW_EMBED_H
#define MOZ_VIEW_EMBED_H

namespace mozilla {
namespace embedlite {

class EmbedLiteView;
class EmbedLiteViewListener
{
public:
    virtual void ViewInitialized() {};
    virtual void LoadFinished() {}
    virtual bool Invalidate() { return false; }
    virtual void Destroyed() {}
};

class EmbedLiteApp;
class EmbedLiteView
{
public:
    EmbedLiteView(EmbedLiteApp* aApp);
    virtual ~EmbedLiteView();

    virtual void SetListener(EmbedLiteViewListener* aListener) { mListener = aListener; }

private:
    friend class EmbedLiteApp;

    void SetImpl(void*);

    EmbedLiteApp* mApp;
    EmbedLiteViewListener* mListener;
    void* mViewImpl;
};

} // namespace embedlite
} // namespace mozilla

#endif
