/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EmbedLiteApp.h"
#include "nsISupports.h"

namespace mozilla {
namespace embedlite {

EmbedLiteApp*
EmbedLiteApp::GetSingleton()
{
    if (!sSingleton) {
        sSingleton = new EmbedLiteApp();
        NS_ASSERTION(sSingleton, "not initialized");
    }
    return sSingleton;
}

EmbedLiteApp::EmbedLiteApp()
{
    sSingleton = this;
}

EmbedLiteApp::~EmbedLiteApp()
{
    sSingleton = NULL;
}

} // namespace embedlite
} // namespace mozilla

mozilla::embedlite::EmbedLiteApp*
XRE_GetEmbedLite()
{
    return mozilla::embedlite::EmbedLiteApp::GetSingleton();
}
