/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GStreamerResourceHandler.h"

#ifdef HAS_NEMO_RESOURCE
#include <QtGui/QGuiApplication>
#define signals Q_SIGNALS
#define slots Q_SLOTS
#include <policy/audio-resource.h>
#include <policy/resource-set.h>
#include "nsThreadUtils.h"

using namespace ResourcePolicy;
#endif

namespace mozilla {

#ifdef HAS_NEMO_RESOURCE
class RegisterAquireMediaTask : public nsRunnable
{
public:
  RegisterAquireMediaTask(GStreamerResourceHandler* handler)
    : mHandler(handler) {}

  NS_IMETHODIMP Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    ResourceSet* set = new ResourcePolicy::ResourceSet("player");
    ResourcePolicy::AudioResource *audioResource = new ResourcePolicy::AudioResource("player");
    audioResource->setProcessID(QCoreApplication::applicationPid());
    audioResource->setStreamTag("media.name", "*");
    set->addResourceObject(audioResource);
    set->addResource(ResourcePolicy::VideoPlaybackType);
    set->acquire();
    mHandler->SetResource(set);

    return NS_OK;
  }
  GStreamerResourceHandler* mHandler;
};

class RegisterDeleteResourceTask : public nsRunnable
{
public:
  RegisterDeleteResourceTask(ResourceSet* deleter)
    : mDeleter(deleter) {}

  NS_IMETHODIMP Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    delete mDeleter;
    return NS_OK;
  }
  ResourceSet* mDeleter;
};
#endif

GStreamerResourceHandler::GStreamerResourceHandler()
  : mResourceSet(nullptr)
  , mLock("GStreamerResourceHandler.mLock")
{
#ifdef HAS_NEMO_RESOURCE
    NS_DispatchToMainThread(new RegisterAquireMediaTask(this), NS_DISPATCH_NORMAL);
#endif
}

GStreamerResourceHandler::~GStreamerResourceHandler()
{
#ifdef HAS_NEMO_RESOURCE
    MutexAutoLock lock(mLock);
    if (mResourceSet)
    {
        NS_DispatchToMainThread(new RegisterDeleteResourceTask(static_cast<ResourceSet*>(mResourceSet)), NS_DISPATCH_NORMAL);
    }
#endif
}

void GStreamerResourceHandler::SetResource(void* set)
{
    MutexAutoLock lock(mLock);
    mResourceSet = set;
}

bool GStreamerResourceHandler::IsDormantNeeded()
{
#ifdef HAS_NEMO_RESOURCE
    return true;
#else
    return false;
#endif
}

bool GStreamerResourceHandler::IsWaitingMediaResources()
{
#ifdef HAS_NEMO_RESOURCE
    MutexAutoLock lock(mLock);
    bool v = mResourceSet ? static_cast<ResourceSet*>(mResourceSet)->hasResourcesGranted() == false : false;
    return v;
#else
    return false;
#endif
}

} // namespace mozilla

