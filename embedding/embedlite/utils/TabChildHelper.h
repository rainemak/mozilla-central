/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __TabChildHelper_h_
#define __TabChildHelper_h_

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "FrameMetrics.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteViewThreadChild;
class TabChildHelper : public nsIObserver
{
public:
    TabChildHelper(EmbedLiteViewThreadChild* aView);
    virtual ~TabChildHelper();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    bool RecvUpdateFrame(const mozilla::layers::FrameMetrics& aFrameMetrics);
    // Wrapper for nsIDOMWindowUtils.setCSSViewport(). This updates some state
    // variables local to this class before setting it.
    void SetCSSViewport(float aX, float aY);

    // Recalculates the display state, including the CSS
    // viewport. This should be called whenever we believe the
    // viewport data on a document may have changed. If it didn't
    // change, this function doesn't do anything.  However, it should
    // not be called all the time as it is fairly expensive.
    void HandlePossibleViewportChange();

private:
    friend class EmbedLiteViewThreadChild;
    EmbedLiteViewThreadChild* mView;
    bool mContentDocumentIsDisplayed;
    mozilla::layers::FrameMetrics mLastMetrics;
    nsIntSize mInnerSize;
    float mOldViewportWidth;
};

}}

#endif

