/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteApp"
#include "EmbedLog.h"

#include "EmbedLiteApp.h"
#include "nsISupports.h"
#include "base/at_exit.h"
#include "mozilla/unused.h"

#include "mozilla/embedlite/EmbedLiteAPI.h"

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
    if (!sSingleton) {
        sSingleton = new EmbedLiteApp();
        NS_ASSERTION(sSingleton, "not initialized");
    }
    return sSingleton;
}

class FakeListener : public EmbedLiteAppListener {};
EmbedLiteApp::EmbedLiteApp()
  : mListener(new FakeListener())
  , mUILoop(NULL)
  , mSubThread(NULL)
  , mEmbedType(EMBED_INVALID)
  , mAppThread(NULL)
  , mViewCreateID(0)
  , mDestroying(false)
  , mRenderType(RENDER_AUTO)
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

void*
EmbedLiteApp::PostTask(EMBEDTaskCallback callback, void* userData, int timeout)
{
    CancelableTask* newTask = NewRunnableFunction(callback, userData);
    if (timeout) {
        mUILoop->PostDelayedTask(FROM_HERE, newTask, timeout);
    } else {
        mUILoop->PostTask(FROM_HERE, newTask);
    }

    return (void*)newTask;
}

void
EmbedLiteApp::CancelTask(void* aTask)
{
    if (aTask) {
        static_cast<CancelableTask*>(aTask)->Cancel();
    }
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
    } else {
        NS_ABORT_IF_FALSE(mListener->StopChildThread(),
                          "StopChildThread must be implemented when ExecuteChildThread defined");
    }
    delete mUILoop;
    mUILoop = NULL;
    mListener->Destroyed();
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

void _FinalStop(EmbedLiteApp* app)
{
    app->Stop();
}

void
EmbedLiteApp::Stop()
{
    LOGT();
    if (!mViews.empty()) {
        std::map<uint32_t, EmbedLiteView*>::iterator it;
        for (it = mViews.begin(); it != mViews.end(); it++) {
            EmbedLiteView* view = it->second;
            delete view;
            it->second = nullptr;
        }
        mDestroying = true;
    } else if (!mDestroying) {
        mDestroying = true;
        mUILoop->PostTask(FROM_HERE,
                          NewRunnableMethod(STHREADAPP(), &EmbedLiteAppThreadParent::SendPreDestroy));
    } else {
        NS_ASSERTION(mUILoop, "Start was not called before stop");
        mUILoop->DoQuit();
    }
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

void
EmbedLiteApp::LoadGlobalStyleSheet(const char* aUri, bool aEnable)
{
    LOGT();
    unused << STHREADAPP()->SendLoadGlobalStyleSheet(nsDependentCString(aUri), aEnable);
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

EmbedLiteView* EmbedLiteApp::GetViewByID(uint32_t id)
{
    std::map<uint32_t, EmbedLiteView*>::iterator it = mViews.find(id);
    return it->second;
}

void
EmbedLiteApp::ChildReadyToDestroy()
{
    LOGT();
    if (mDestroying) {
        mUILoop->PostTask(FROM_HERE,
                          NewRunnableFunction(&_FinalStop, this));
    }
}

void
EmbedLiteApp::ViewDestroyed(uint32_t id)
{
    LOGT("id:%i", id);
    std::map<uint32_t, EmbedLiteView*>::iterator it = mViews.find(id);
    mViews.erase(it);
    if (mDestroying && mViews.empty()) {
        mUILoop->PostTask(FROM_HERE,
                          NewRunnableMethod(STHREADAPP(), &EmbedLiteAppThreadParent::SendPreDestroy));
    }
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
    delete view;
    it->second = nullptr;
}

void
EmbedLiteApp::SetIsAccelerated(bool aIsAccelerated)
{
#ifdef GL_PROVIDER_EGL
    if (aIsAccelerated) {
        mRenderType = RENDER_HW;
    } else
#endif
    {
        mRenderType = RENDER_SW;
    }
}

} // namespace embedlite
} // namespace mozilla

mozilla::embedlite::EmbedLiteApp*
XRE_GetEmbedLite()
{
    return mozilla::embedlite::EmbedLiteApp::GetInstance();
}
