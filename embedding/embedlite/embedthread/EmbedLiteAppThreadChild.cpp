/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteAppThreadChild"
#include "EmbedLog.h"

#include "EmbedLiteAppThreadChild.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIWindowWatcher.h"
#include "WindowCreator.h"
#include "nsIURI.h"
#include "nsIStyleSheetService.h"
#include "nsNetUtil.h"

#include "EmbedLiteAppThreadParent.h"
#include "EmbedLiteViewThreadChild.h"
#include "mozilla/unused.h"
#include "mozilla/layers/ImageBridgeChild.h"
#include "EmbedLiteModulesService.h"
#include "EmbedLiteAppService.h"

using namespace base;
using namespace mozilla::ipc;
using namespace mozilla::layers;

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
    RecvSetBoolPref(nsCString("layers.offmainthreadcomposition.enabled"), true);
    mModulesService = new EmbedLiteModulesService();
    mModulesService->Init();
    SendInitialized();
}

EmbedLiteAppService*
EmbedLiteAppThreadChild::AppService()
{
    nsCOMPtr<nsIEmbedAppService> service =
        do_GetService("@mozilla.org/embedlite-app-service;1");
    return static_cast<EmbedLiteAppService*>(service.get());
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
    LOGT("id:%u", id);
    EmbedLiteViewThreadChild* view = new EmbedLiteViewThreadChild(id);
    mWeakViewMap[id] = view;
    return view;
}

bool
EmbedLiteAppThreadChild::DeallocPEmbedLiteView(PEmbedLiteViewChild* actor)
{
    LOGT();
    std::map<uint32_t, EmbedLiteViewThreadChild*>::iterator it;
    for (it = mWeakViewMap.begin(); it != mWeakViewMap.end(); ++it) {
        if (actor == it->second)
            break;
    }
    mWeakViewMap.erase(it);
    delete actor;
    return true;
}

EmbedLiteViewThreadChild*
EmbedLiteAppThreadChild::GetViewByID(uint32_t aId)
{
    return mWeakViewMap[aId];
}

bool
EmbedLiteAppThreadChild::RecvPreDestroy()
{
    LOGT();
    ImageBridgeChild::ShutDown();
    SendReadyToShutdown();
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

bool
EmbedLiteAppThreadChild::RecvLoadGlobalStyleSheet(const nsCString& uri, const bool& aEnable)
{
    LOGT("uri:%s, enable:%i", uri.get(), aEnable);
    nsCOMPtr<nsIStyleSheetService> styleSheetService =
        do_GetService("@mozilla.org/content/style-sheet-service;1");
    NS_ENSURE_TRUE(styleSheetService, false);
    nsCOMPtr<nsIURI> nsuri;
    NS_NewURI(getter_AddRefs(nsuri), uri);
    NS_ENSURE_TRUE(nsuri, false);
    if (aEnable) {
        styleSheetService->LoadAndRegisterSheet(nsuri, nsIStyleSheetService::AGENT_SHEET);
    } else {
        styleSheetService->UnregisterSheet(nsuri, nsIStyleSheetService::AGENT_SHEET);
    }
    return true;
}

bool
EmbedLiteAppThreadChild::RecvAsyncMessage(const nsString& message, const nsString& messageName)
{
    LOGNI("msg:%s, data:%s", NS_ConvertUTF16toUTF8(message).get(), NS_ConvertUTF16toUTF8(messageName).get());
    return true;
}

} // namespace embedlite
} // namespace mozilla

