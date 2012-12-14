/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteAppThreadChild"

#include "EmbedLiteAppThreadChild.h"
#include "EmbedLog.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIWindowWatcher.h"
#include "WindowCreator.h"

#include "EmbedLiteAppThreadParent.h"
#include "EmbedLiteViewThreadChild.h"
#include "mozilla/unused.h"

using namespace base;
using namespace mozilla::ipc;

namespace mozilla {
namespace embedlite {

static EmbedLiteAppThreadChild* sAppThreadChild = nullptr;

EmbedLiteAppThreadChild*
EmbedLiteAppThreadChild::GetInstance()
{
    return sAppThreadChild;
}

EmbedLiteAppThreadChild::EmbedLiteAppThreadChild(MessageLoop* aParentLoop)
  : mParentLoop(aParentLoop)
{
    LOGT();
    sAppThreadChild = this;
}

EmbedLiteAppThreadChild::~EmbedLiteAppThreadChild()
{
    LOGT();
    sAppThreadChild = nullptr;
}

void
EmbedLiteAppThreadChild::Init(EmbedLiteAppThreadParent* aParent)
{
    LOGT();
    InitWindowWatcher();
    AsyncChannel *parentChannel = aParent->GetIPCChannel();
    AsyncChannel::Side childSide = mozilla::ipc::AsyncChannel::Child;
    Open(parentChannel, mParentLoop, childSide);
    SendInitialized();
}

void
EmbedLiteAppThreadChild::InitWindowWatcher()
{
    // create an nsWindowCreator and give it to the WindowWatcher service
    nsCOMPtr<nsIWindowCreator> creator(new WindowCreator());
    if (!creator) {
        LOGE("Out of memory");
        return;
    }
    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    if (!wwatch) {
        LOGE("Fail to get watcher service");
        return;
    }
    LOGT("Created window watcher!");
    wwatch->SetWindowCreator(creator);
}

void
EmbedLiteAppThreadChild::ActorDestroy(ActorDestroyReason aWhy)
{
    LOGT("reason:%i", aWhy);
}

bool
EmbedLiteAppThreadChild::RecvCreateView(const uint32_t& id)
{
    LOGT("id:%u", id);
    unused << SendPEmbedLiteViewConstructor(id);
    return true;
}

PEmbedLiteViewChild*
EmbedLiteAppThreadChild::AllocPEmbedLiteView(const uint32_t& id)
{
    LOGT();
    return new EmbedLiteViewThreadChild(id);
}

bool
EmbedLiteAppThreadChild::DeallocPEmbedLiteView(PEmbedLiteViewChild* actor)
{
    LOGT();
    delete actor;
    return true;
}

bool
EmbedLiteAppThreadChild::RecvSetBoolPref(const nsCString& aName, const bool& aValue)
{
    LOGC("EmbedPrefs", "n:%s, v:%i", aName.get(), aValue);
    nsresult rv;
    nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    if (NS_FAILED(rv)) {
        LOGE("Cannot get prefService");
        return false;
    }

    pref->SetBoolPref(aName.get(), aValue);
    return true;
}

bool EmbedLiteAppThreadChild::RecvSetCharPref(const nsCString& aName, const nsCString& aValue)
{
    LOGC("EmbedPrefs", "n:%s, v:%s", aName.get(), aValue.get());
    nsresult rv;
    nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    if (NS_FAILED(rv)) {
        LOGE("Cannot get prefService");
        return false;
    }

    pref->SetCharPref(aName.get(), aValue.get());
    return true;
}

bool EmbedLiteAppThreadChild::RecvSetIntPref(const nsCString& aName, const int& aValue)
{
    LOGC("EmbedPrefs", "n:%s, v:%i", aName.get(), aValue);
    nsresult rv;
    nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    if (NS_FAILED(rv)) {
        LOGE("Cannot get prefService");
        return false;
    }

    pref->SetIntPref(aName.get(), aValue);
    return true;
}

} // namespace embedlite
} // namespace mozilla

