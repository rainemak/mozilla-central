/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EmbedLiteApp.h"
#include "nsISupports.h"

#include "mozilla/embedlite/EmbedLiteAPI.h"
#include "prlog.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gEmbedLiteAppLog;
#define LOG(...) PR_LOG(gEmbedLiteAppLog, PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...) do {} while (0)
#endif

namespace mozilla {
namespace embedlite {

EmbedLiteApp* EmbedLiteApp::sSingleton = nullptr;

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
#ifdef PR_LOGGING
    if (!gEmbedLiteAppLog) {
        gEmbedLiteAppLog = PR_NewLogModule("EmbedLiteApp");
    }
#endif

    sSingleton = this;
}

EmbedLiteApp::~EmbedLiteApp()
{
    sSingleton = NULL;
}

void
EmbedLiteApp::SetListener(EmbedLiteAppListener* aListener)
{
    LOG("EmbedLiteApp::SetListener %p", aListener);
    mListener = aListener;
}

bool
EmbedLiteApp::Start(EmbedType aEmbedType)
{
    return false;
}

void
EmbedLiteApp::Stop()
{
}

void
EmbedLiteApp::SetBoolPref(const char* aName, bool aValue)
{
//    mImpl->SetBoolPref(aName, aValue);
}

void
EmbedLiteApp::SetCharPref(const char* aName, const char* aValue)
{
//    mImpl->SetCharPref(aName, aValue);
}

void
EmbedLiteApp::SetIntPref(const char* aName, int aValue)
{
//    mImpl->SetIntPref(aName, aValue);
}

} // namespace embedlite
} // namespace mozilla

mozilla::embedlite::EmbedLiteApp*
XRE_GetEmbedLite()
{
    return mozilla::embedlite::EmbedLiteApp::GetSingleton();
}
