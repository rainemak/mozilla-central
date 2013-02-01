/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsServiceManagerUtils.h"
#include "nsICategoryManager.h"
#include "mozilla/ModuleUtils.h"
#include "nsIAppStartupNotifier.h"
#include "EmbedChromeListener.h"
#include "nsNetCID.h"
#include <iostream>

// XPCOMGlueStartup
#include "nsXPCOMGlue.h"

/* ===== XPCOM registration stuff ======== */

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(EmbedChromeListener, Init)

NS_DEFINE_NAMED_CID(NS_EMBED_CHROME_SERVICE_CID);

static const mozilla::Module::CIDEntry kEMBEDCHROMECIDs[] = {
    { &kNS_EMBED_CHROME_SERVICE_CID, false, NULL, EmbedChromeListenerConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kEMBEDCHROMEContracts[] = {
    { NS_EMBED_CHROME_CONTRACTID, &kNS_EMBED_CHROME_SERVICE_CID },
    { NULL }
};

static const mozilla::Module::CategoryEntry kEMBEDCHROMECategories[] = {
    { APPSTARTUP_CATEGORY, NS_EMBED_CHROME_SERVICE_CLASSNAME, NS_EMBED_CHROME_CONTRACTID },
    { NULL }
};

static nsresult
EmbedChrome_Initialize()
{
    XPCOMGlueStartup(getenv("XRE_LIBXPCOM_PATH"));
    return NS_OK;
}

static void
EmbedChrome_Shutdown()
{
}

static const mozilla::Module kEMBEDCHROMEModule = {
    mozilla::Module::kVersion,
    kEMBEDCHROMECIDs,
    kEMBEDCHROMEContracts,
    kEMBEDCHROMECategories,
    NULL,
    EmbedChrome_Initialize,
    EmbedChrome_Shutdown
};

NSMODULE_DEFN(nsEmbedChromeModule) = &kEMBEDCHROMEModule;
