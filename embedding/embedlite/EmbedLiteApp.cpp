/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteApp"

#include "EmbedLiteApp.h"
#include "nsISupports.h"
#include "base/at_exit.h"

#include "mozilla/embedlite/EmbedLiteAPI.h"
#include "EmbedLog.h"
#include "EmbedLiteUILoop.h"
#include "EmbedLiteSubThread.h"
#include "GeckoLoader.h"

namespace mozilla {
namespace embedlite {

EmbedLiteApp* EmbedLiteApp::sSingleton = nullptr;

EmbedLiteApp*
EmbedLiteApp::GetSingleton()
{
    LOGT();
    if (!sSingleton) {
        sSingleton = new EmbedLiteApp();
        NS_ASSERTION(sSingleton, "not initialized");
    }
    return sSingleton;
}

EmbedLiteApp::EmbedLiteApp()
  : mListener(NULL)
  , mUILoop(NULL)
  , mSubThread(NULL)
{
    LOGT();
    sSingleton = this;
}

EmbedLiteApp::~EmbedLiteApp()
{
    LOGT();
    NS_ASSERTION(!mUILoop, "Main Loop not stopped before destroy");
    NS_ASSERTION(!mSubThread, "Thread not stopped/destroyed before destroy");
    sSingleton = NULL;
}

void
EmbedLiteApp::SetListener(EmbedLiteAppListener* aListener)
{
    LOGT();
    mListener = aListener;
}

void
EmbedLiteApp::StartChild(EmbedLiteApp* aApp, EmbedType aEmbedType)
{
    LOGT();
    if (!aApp->GetListener() || !aApp->GetListener()->ExecuteChildThread()) {
        if (aEmbedType == EMBED_THREAD) {
            aApp->mSubThread = new EmbedLiteSubThread(aApp);
            if (!aApp->mSubThread->StartEmbedThread()) {
                LOGE("Failed to start child thread");
            }
        }
    }
}

bool
EmbedLiteApp::Start(EmbedType aEmbedType)
{
    LOGT("Type: %s", aEmbedType == EMBED_THREAD ? "Thread" : "Process");
    NS_ASSERTION(!mUILoop, "Start called twice");
    base::AtExitManager exitManager;
    mUILoop = new EmbedLiteUILoop();
    mUILoop->PostTask(FROM_HERE,
                      NewRunnableFunction(&EmbedLiteApp::StartChild, this, aEmbedType));
    mUILoop->StartLoop();
    if (mSubThread) {
        mSubThread->Stop();
        mSubThread = NULL;
    }
    delete mUILoop;
    mUILoop = NULL;
    return true;
}

bool
EmbedLiteApp::StartChildThread()
{
    LOGT("mUILoop:%p, current:%p", mUILoop, MessageLoop::current());
    NS_ASSERTION(MessageLoop::current(),
                 "Current message loop must be null and not equals to mUILoop");
    GeckoLoader::InitEmbedding("mozembed");
    return !MessageLoop::current();
}

bool
EmbedLiteApp::StopChildThread()
{
    LOGT("mUILoop:%p, current:%p", mUILoop, MessageLoop::current());
    if (mSubThread || !mUILoop || !MessageLoop::current() ||
        mUILoop == MessageLoop::current()) {
        NS_ERROR("Wrong thread? StartChildThread called? Stop() already called?");
        return false;
    }
    GeckoLoader::TermEmbedding();
    return true;
}

void
EmbedLiteApp::Stop()
{
    LOGT();
    NS_ASSERTION(mUILoop, "Start was not called before stop");
    mUILoop->DoQuit();
}

void
EmbedLiteApp::SetBoolPref(const char* aName, bool aValue)
{
    LOGNI("n:%s, v:%i", aName, aValue);
}

void
EmbedLiteApp::SetCharPref(const char* aName, const char* aValue)
{
    LOGNI("n:%s, v:%s", aName, aValue);
}

void
EmbedLiteApp::SetIntPref(const char* aName, int aValue)
{
    LOGNI("n:%s, v:%i", aName, aValue);
}

} // namespace embedlite
} // namespace mozilla

mozilla::embedlite::EmbedLiteApp*
XRE_GetEmbedLite()
{
    return mozilla::embedlite::EmbedLiteApp::GetSingleton();
}
