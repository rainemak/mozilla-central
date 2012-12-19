/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_VIEW_EMBED_H
#define MOZ_VIEW_EMBED_H

#include "mozilla/RefPtr.h"
#include "nsStringGlue.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteViewImplIface;
class EmbedLiteView;
class EmbedKineticListener;
class EmbedKineticModule;
class EmbedLiteViewListener
{
public:
    // View intialized and ready for API calls
    virtual void ViewInitialized() {}
    // View finally destroyed and deleted
    virtual void Destroyed() {}
    // Invalidate notification
    virtual bool Invalidate() { return false; }

    virtual void OnTitleChanged(const PRUnichar* aTitle) {}
    virtual void OnLocationChanged(const char* aLocation, bool aCanGoBack, bool aCanGoForward) {}
    virtual void OnLoadStarted(const char* aLocation) {}
    virtual void OnLoadFinished(void) {}
    virtual void OnLoadRedirect(void) {}
    virtual void OnLoadProgress(int32_t aProgress, int32_t aCurTotal, int32_t aMaxTotal) {}
    virtual void OnSecurityChanged(const char* aStatus, unsigned int aState) {}
    virtual void OnFirstPaint(int32_t aX, int32_t aY) {}
    virtual void OnContentLoaded(const PRUnichar* aDocURI) {}
    virtual void OnLinkAdded(const PRUnichar* aHref, const PRUnichar* aCharset, const PRUnichar* aTitle, const PRUnichar* aRel, const PRUnichar* aSizes, const PRUnichar* aType) {}
    virtual void OnWindowOpenClose(const PRUnichar* aType) {}
    virtual void OnPopupBlocked(const char* aSpec, const char* aCharset, const PRUnichar* aPopupFeatures, const PRUnichar* aPopupWinName) {}
    virtual void OnPageShowHide(const PRUnichar* aType, bool aPersisted) {}
    virtual void OnScrolledAreaChanged(unsigned int aWidth, unsigned int aHeight) {}
    virtual void OnScrollChanged(int32_t offSetX, int32_t offSetY) {}
    virtual void OnObserve(const char* aTopic, const PRUnichar* aData) {}
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

    virtual void MousePress(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers);
    virtual void MouseRelease(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers);
    virtual void MouseMove(int x, int y, int mstime, unsigned int buttons, unsigned int modifiers);

    // Scrolling mode, enable internal smart scroll/zoom component
    virtual void SetScrollingMode(bool aEnabled) { mScrollingMode = aEnabled; }
    virtual bool GetScrollingMode() { return mScrollingMode; }

    virtual void SetViewSize(int width, int height);

    // Scroll/Zoom API
    virtual bool ScrollBy(int aDX, int aDY, bool aDoOverflow = false);

    // PNG Decoded data
    virtual char* GetImageAsURL(int aWidth = -1, int aHeight = -1);

private:
    friend class EmbedLiteViewThreadParent;
    friend class EmbedLiteCompositorParent;
    void SetImpl(EmbedLiteViewImplIface*);
    EmbedLiteViewImplIface* GetImpl();

    EmbedLiteApp* mApp;
    EmbedLiteViewListener* mListener;
    EmbedLiteViewImplIface* mViewImpl;
    bool mScrollingMode;
    RefPtr<EmbedKineticListener> mKineticListener;
    RefPtr<EmbedKineticModule> mKinetic;
};

} // namespace embedlite
} // namespace mozilla

#endif
