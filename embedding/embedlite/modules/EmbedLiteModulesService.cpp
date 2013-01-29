/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteModulesService"
#include "EmbedLog.h"

#include "EmbedLiteModulesService.h"

#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsIObserverService.h"
#include "nsStringGlue.h"
#include "nsIChannel.h"
#include "EmbedPromptService.h"
#include "EmbedLiteAppService.h"

#include "nsIComponentRegistrar.h"
#include "nsIComponentManager.h"
#include "mozilla/GenericFactory.h"
#include "mozilla/ModuleUtils.h"
#include "nsComponentManagerUtils.h"
#include "mozilla/Preferences.h"


#include "nsIDOMWindowUtils.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWebNavigation.h"
#include "nsIInterfaceRequestorUtils.h"

// nsCxPusher
#include "nsContentUtils.h"
#include "nsISupportsPrimitives.h"

#include "EmbedLiteViewThreadChild.h"

using namespace mozilla::embedlite;

NS_GENERIC_FACTORY_CONSTRUCTOR(EmbedPromptFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR(EmbedLiteAppService)

EmbedLiteModulesService::EmbedLiteModulesService()
{
}

EmbedLiteModulesService::~EmbedLiteModulesService()
{
}

NS_IMPL_ISUPPORTS2(EmbedLiteModulesService, nsIObserver, nsSupportsWeakReference)

nsresult
EmbedLiteModulesService::Init()
{
    nsCOMPtr<nsIComponentRegistrar> cr;
    nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(cr));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    nsCOMPtr<nsIComponentManager> cm;
    rv = NS_GetComponentManager (getter_AddRefs (cm));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    if (Preferences::GetBool("embedlite.prompt_enabled", true)) {

        nsCOMPtr<nsIFactory> f = new mozilla::GenericFactory(EmbedPromptFactoryConstructor);
        if (!f) {
            NS_WARNING("Unable to create factory for component");
            return NS_ERROR_FAILURE;
        }

        nsCOMPtr<nsIFactory> oldFactory = do_GetClassObject("@mozilla.org/prompter;1");
        if (oldFactory) {
            nsCID* cid = NULL;
            rv = cr->ContractIDToCID("@mozilla.org/prompter;1", &cid);
            if (!NS_FAILED(rv)) {
                rv = cr->UnregisterFactory(*cid, oldFactory.get());
                NS_Free(cid);
                if (NS_FAILED(rv)) {
                    return NS_ERROR_FAILURE;
                }
            }
        }
        nsCID promptCID = EMBED_LITE_PROMPT_SERVICE_CID;
        rv = cr->RegisterFactory(promptCID, "EmbedLite Prompt",
                                 "@mozilla.org/prompter;1", f);

        NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to register factory for component");
    }

    {
        nsCOMPtr<nsIFactory> f = new mozilla::GenericFactory(EmbedLiteAppServiceConstructor);
        if (!f) {
            NS_WARNING("Unable to create factory for component");
            return NS_ERROR_FAILURE;
        }

        nsCID appCID = NS_EMBED_LITE_APP_SERVICE_CID;
        rv = cr->RegisterFactory(appCID, NS_EMBED_LITE_APP_SERVICE_CLASSNAME,
                                 NS_EMBED_LITE_APP_CONTRACTID, f);
    }

    nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);

    if (observerService) {
        observerService->AddObserver(this,
                                     "outer-window-destroyed",
                                     true);
    }

    return NS_OK;
}

NS_IMETHODIMP
EmbedLiteModulesService::Observe(nsISupports *aSubject,
                                 const char *aTopic,
                                 const PRUnichar *aData)
{
    if (!strcmp(aTopic, "outer-window-destroyed")) {
        nsCOMPtr<nsISupportsPRUint64> wrapper = do_QueryInterface(aSubject);
        if (wrapper) {
            uint64_t id = 0;
            wrapper->GetData(&id);
            mViewWeakMap.erase(id);
        }
    }
    return NS_OK;
}

/*
void
EmbedLiteModulesService::RegisterView(EmbedLiteViewThreadChild* aView)
{
    nsCxPusher pusher;
    pusher.PushNull();
    nsCOMPtr<nsIDOMWindow> window = do_GetInterface(aView->mWebNavigation);
    nsCOMPtr<nsIDOMWindow> top;
    window->GetTop(getter_AddRefs(top));
    if (top)
        window = top;

    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);
    uint64_t OuterWindowID = 0;
    utils->GetOuterWindowID(&OuterWindowID);
    mViewWeakMap[OuterWindowID] = aView;
}

void
EmbedLiteModulesService::UnregisterView(EmbedLiteViewThreadChild* aView)
{
    nsCxPusher pusher;
    pusher.PushNull();
    std::map<uint64_t, EmbedLiteViewThreadChild*>::iterator it;
    for (it = mViewWeakMap.begin(); it != mViewWeakMap.end(); ++it) {
        if (aView == it->second) {
            break;
        }
    }
    mViewWeakMap.erase(it);
}

EmbedLiteViewThreadChild*
EmbedLiteModulesService::GetViewForWindow(nsIDOMWindow* aParent)
{
    nsCxPusher pusher;
    pusher.PushNull();
    nsCOMPtr<nsIDOMWindow> window;
    nsCOMPtr<nsIWebNavigation> navNav(do_GetInterface(aParent));
    nsCOMPtr<nsIDocShellTreeItem> navItem(do_QueryInterface(navNav));
    if (navItem) {
        nsCOMPtr<nsIDocShellTreeItem> rootItem;
        navItem->GetRootTreeItem(getter_AddRefs(rootItem));
        nsCOMPtr<nsIDOMWindow> rootWin(do_GetInterface(rootItem));
        rootWin->GetTop(getter_AddRefs(window));
    }
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);
    uint64_t OuterWindowID = 0;
    utils->GetOuterWindowID(&OuterWindowID);
    return mViewWeakMap[OuterWindowID];
}
*/
