/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsServiceManagerUtils.h"
#include "nsICategoryManager.h"
#include "mozilla/ModuleUtils.h"
#include "nsIAppStartupNotifier.h"
#include "EmbedChildListener.h"
#include "nsNetCID.h"
#include "mozilla/embedlite/EmbedLog.h"
#include <iostream>

// XPCOMGlueStartup
#include "nsXPCOMGlue.h"

/* ===== XPCOM registration stuff ======== */

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(EmbedChildListener, Init)

NS_DEFINE_NAMED_CID(NS_EMBED_CHILD_SERVICE_CID);

static const mozilla::Module::CIDEntry kEMBEDCHILDCIDs[] = {
    { &kNS_EMBED_CHILD_SERVICE_CID, false, NULL, EmbedChildListenerConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kEMBEDCHILDContracts[] = {
    { NS_EMBED_CHILD_CONTRACTID, &kNS_EMBED_CHILD_SERVICE_CID },
    { NULL }
};

static const mozilla::Module::CategoryEntry kEMBEDCHILDCategories[] = {
    { APPSTARTUP_CATEGORY, "Embed Child Module", NS_EMBED_CHILD_CONTRACTID },
    { NULL }
};

static nsresult
EmbedChild_Initialize()
{
    LOGT();
    XPCOMGlueStartup(getenv("XRE_LIBXPCOM_PATH"));
    return NS_OK;
}

static void
EmbedChild_Shutdown()
{
    LOGT();
}

static const mozilla::Module kEMBEDCHILDModule = {
    mozilla::Module::kVersion,
    kEMBEDCHILDCIDs,
    kEMBEDCHILDContracts,
    kEMBEDCHILDCategories,
    NULL,
    EmbedChild_Initialize,
    EmbedChild_Shutdown
};

NSMODULE_DEFN(nsEmbedChildModule) = &kEMBEDCHILDModule;
