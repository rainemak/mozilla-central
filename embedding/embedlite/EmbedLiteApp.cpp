/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteApp"

#include "EmbedLiteApp.h"
#include "nsISupports.h"
#include "base/at_exit.h"
#include "mozilla/unused.h"

#include "mozilla/embedlite/EmbedLiteAPI.h"
#include "EmbedLog.h"
#include "EmbedLiteUILoop.h"
#include "EmbedLiteSubThread.h"
#include "GeckoLoader.h"

#include "EmbedLiteAppThread.h"
#include "EmbedLiteAppThreadParent.h"
#include "EmbedLiteView.h"

#define STHREADAPP EmbedLiteAppThreadParent::GetInstance

namespace mozilla {
namespace embedlite {

EmbedLiteApp* EmbedLiteApp::sSingleton = nullptr;

EmbedLiteApp*
EmbedLiteApp::GetInstance()
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
  , mEmbedType(EMBED_INVALID)
  , mAppThread(NULL)
  , mViewCreateID(0)
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
EmbedLiteApp::StartChild(EmbedLiteApp* aApp)
{
    LOGT();
    if (aApp->mEmbedType == EMBED_THREAD) {
        if (!aApp->GetListener() ||
            !aApp->GetListener()->ExecuteChildThread()) {
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
    mEmbedType = aEmbedType;
    base::AtExitManager exitManager;
    mUILoop = new EmbedLiteUILoop();
    mUILoop->PostTask(FROM_HERE,
                      NewRunnableFunction(&EmbedLiteApp::StartChild, this));
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
    NS_ENSURE_TRUE(mEmbedType == EMBED_THREAD, false);
    LOGT("mUILoop:%p, current:%p", mUILoop, MessageLoop::current());
    NS_ASSERTION(MessageLoop::current() != mUILoop,
                 "Current message loop must be null and not equals to mUILoop");
    GeckoLoader::InitEmbedding("mozembed");
    mAppThread = new EmbedLiteAppThread(mUILoop);
    return true;
}

bool
EmbedLiteApp::StopChildThread()
{
    NS_ENSURE_TRUE(mEmbedType == EMBED_THREAD, false);
    LOGT("mUILoop:%p, current:%p", mUILoop, MessageLoop::current());
    if (!mUILoop || !MessageLoop::current() ||
        mUILoop == MessageLoop::current()) {
        NS_ERROR("Wrong thread? StartChildThread called? Stop() already called?");
        return false;
    }
    mAppThread->Destroy();
    mAppThread = nullptr;
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
    unused << STHREADAPP()->SendSetBoolPref(nsCString(aName), aValue);
}

void
EmbedLiteApp::SetCharPref(const char* aName, const char* aValue)
{
    unused << STHREADAPP()->SendSetCharPref(nsCString(aName), nsCString(aValue));
}

void
EmbedLiteApp::SetIntPref(const char* aName, int aValue)
{
    unused << STHREADAPP()->SendSetIntPref(nsCString(aName), aValue);
}

EmbedLiteView*
EmbedLiteApp::CreateView()
{
    LOGT();
    EmbedLiteView* view = new EmbedLiteView(this);
    mViews[mViewCreateID] = view;
    unused << STHREADAPP()->SendCreateView(mViewCreateID);
    mViewCreateID++;
    return view;
}

void EmbedLiteApp::RegisterViewImpl(void* view, uint32_t id)
{
    std::map<uint32_t, EmbedLiteView*>::iterator it = mViews.find(id);
    it->second->SetImpl(view);
}

void EmbedLiteApp::DestroyView(EmbedLiteView* aView)
{
    LOGT();
    std::map<uint32_t, EmbedLiteView*>::iterator it;
    for (it = mViews.begin(); it != mViews.end(); it++) {
        if (it->second == aView)
            break;
    }
    EmbedLiteView* view = it->second;
    mViews.erase(it);
    delete view;
}

} // namespace embedlite
} // namespace mozilla

mozilla::embedlite::EmbedLiteApp*
XRE_GetEmbedLite()
{
    return mozilla::embedlite::EmbedLiteApp::GetInstance();
}
