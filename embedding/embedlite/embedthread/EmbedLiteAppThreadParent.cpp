/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteAppThreadParent"
#include "EmbedLog.h"

#include "EmbedLiteViewThreadParent.h"
#include "EmbedLiteAppThreadParent.h"
#include "EmbedLiteApp.h"

#include "mozilla/unused.h"
#include "EmbedLiteCompositorParent.h"

using namespace base;
using namespace mozilla::ipc;

namespace mozilla {
namespace embedlite {

static EmbedLiteAppThreadParent* sAppThreadParent = nullptr;

EmbedLiteAppThreadParent*
EmbedLiteAppThreadParent::GetInstance()
{
  return sAppThreadParent;
}

EmbedLiteAppThreadParent::EmbedLiteAppThreadParent()
  : mApp(EmbedLiteApp::GetInstance())
{
  LOGT();
  MOZ_COUNT_CTOR(EmbedLiteAppThreadParent);
  sAppThreadParent = this;
}

EmbedLiteAppThreadParent::~EmbedLiteAppThreadParent()
{
  LOGT();
  MOZ_COUNT_DTOR(EmbedLiteAppThreadParent);
  sAppThreadParent = nullptr;
}

bool
EmbedLiteAppThreadParent::RecvInitialized()
{
  LOGT();
  PR_SetEnv("MOZ_USE_OMTC=1");
  mozilla::layers::CompositorParent::StartUpWithExistingThread(MessageLoop::current(), PlatformThread::CurrentId());
  mApp->GetListener()->Initialized();
  bool accel = mApp->IsAccelerated();
  SetBoolPref("layers.acceleration.disabled", !accel);
  SetBoolPref("layers.acceleration.force-enabled", accel);
  SetBoolPref("layers.async-video.enabled", accel && getenv("DISABLE_ASYNC_VIDEO") == 0);
  SetBoolPref("gfx.use_tiled_thebes", accel && getenv("DISABLE_TILED") == 0);
  SetBoolPref("egl.use_backing_surface", accel && getenv("DISABLE_BACKING") == 0);
  SetBoolPref("layers.reuse-invalid-tiles", getenv("DISABLE_REUSE_TILES") != 0);
  return true;
}

bool
EmbedLiteAppThreadParent::RecvReadyToShutdown()
{
  LOGT();
  mApp->ChildReadyToDestroy();
  return true;
}

void
EmbedLiteAppThreadParent::ActorDestroy(ActorDestroyReason aWhy)
{
  LOGT("reason:%i", aWhy);
}

PEmbedLiteViewParent*
EmbedLiteAppThreadParent::AllocPEmbedLiteView(const uint32_t& id)
{
  LOGT();
  EmbedLiteViewThreadParent* tview = new EmbedLiteViewThreadParent(id);
  return tview;
}

bool
EmbedLiteAppThreadParent::DeallocPEmbedLiteView(PEmbedLiteViewParent* actor)
{
  LOGT();
  delete actor;
  return true;
}

void EmbedLiteAppThreadParent::SetBoolPref(const char* aName, bool aValue)
{
  unused << SendSetBoolPref(nsDependentCString(aName), aValue);
}

void EmbedLiteAppThreadParent::SetCharPref(const char* aName, const char* aValue)
{
  unused << SendSetCharPref(nsDependentCString(aName), nsDependentCString(aValue));
}

void EmbedLiteAppThreadParent::SetIntPref(const char* aName, int aValue)
{
  unused << SendSetIntPref(nsDependentCString(aName), aValue);
}

bool
EmbedLiteAppThreadParent::RecvObserve(const nsCString& topic,
                                      const nsString& data)
{
  LOGT("topic:%s", topic.get());
  mApp->GetListener()->OnObserve(topic.get(), data.get());
  return true;
}

} // namespace embedlite
} // namespace mozilla

