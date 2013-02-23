/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsServiceManagerUtils.h"
#include "nsICategoryManager.h"
#include "mozilla/ModuleUtils.h"
#include "nsIAppStartupNotifier.h"
#include "EmbedHistoryListener.h"
#include "nsNetCID.h"
#include "nsDocShellCID.h"
#include <iostream>

// XPCOMGlueStartup
#include "nsXPCOMGlue.h"

/* ===== XPCOM registration stuff ======== */

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(EmbedHistoryListener, EmbedHistoryListener::GetSingleton)
NS_DEFINE_NAMED_CID(NS_EMBED_HISTORY_SERVICE_CID);

static const mozilla::Module::CIDEntry kEMBEDHISTORYCIDs[] = {
    { &kNS_EMBED_HISTORY_SERVICE_CID, false, NULL, EmbedHistoryListenerConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kEMBEDHISTORYContracts[] = {
    { NS_IHISTORY_CONTRACTID, &kNS_EMBED_HISTORY_SERVICE_CID },
    { NULL }
};

static nsresult
EmbedHistory_Initialize()
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
//    XPCOMGlueStartup(getenv("XRE_LIBXPCOM_PATH"));
    return NS_OK;
}

static void
EmbedHistory_Shutdown()
{
    printf(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

static const mozilla::Module kEMBEDHISTORYModule = {
    mozilla::Module::kVersion,
    kEMBEDHISTORYCIDs,
    kEMBEDHISTORYContracts,
    NULL,
    NULL,
    EmbedHistory_Initialize,
    EmbedHistory_Shutdown
};

NSMODULE_DEFN(nsEmbedHistoryModule) = &kEMBEDHISTORYModule;
