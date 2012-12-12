/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DirProvider.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "EmbedLog.h"

nsIDirectoryServiceProvider* DirProvider::sAppFileLocProvider = 0;
nsCOMPtr<nsIFile> DirProvider::sProfileDir = 0;
nsCOMPtr<nsIFile> DirProvider::sGREDir = 0;
nsISupports* DirProvider::sProfileLock = 0;

NS_IMPL_QUERY_INTERFACE2(DirProvider,
                         nsIDirectoryServiceProvider,
                         nsIDirectoryServiceProvider2)

NS_IMETHODIMP_(nsrefcnt)
DirProvider::AddRef()
{
    return 1;
}

NS_IMETHODIMP_(nsrefcnt)
DirProvider::Release()
{
    return 1;
}

NS_IMETHODIMP
DirProvider::GetFile(const char* aKey, bool* aPersist,
                     nsIFile* *aResult)
{
    if (sAppFileLocProvider) {
        nsresult rv = sAppFileLocProvider->GetFile(aKey, aPersist, aResult);
        if (NS_SUCCEEDED(rv))
            return rv;
    }

    if (sGREDir && !strcmp(aKey, NS_GRE_DIR)) {
        *aPersist = true;
        return sGREDir->Clone(aResult);
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_USER_PROFILE_50_DIR)) {
        *aPersist = true;
        return sProfileDir->Clone(aResult);
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_USER_PROFILE_LOCAL_50_DIR)) {
        *aPersist = true;
        return sProfileDir->Clone(aResult);
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_PROFILE_DIR_STARTUP)) {
        *aPersist = true;
        return sProfileDir->Clone(aResult);
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_CACHE_PARENT_DIR)) {
        *aPersist = true;
        return sProfileDir->Clone(aResult);
    }

    LOGT("Failed to GetFile: key:%s\n", aKey);
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DirProvider::GetFiles(const char* aKey,
                      nsISimpleEnumerator* *aResult)
{
    nsCOMPtr<nsIDirectoryServiceProvider2>
        dp2(do_QueryInterface(sAppFileLocProvider));

    if (!dp2) {
        LOGT("Failed to GetFiles: key:%s\n", aKey);
        return NS_ERROR_FAILURE;
    }

    return dp2->GetFiles(aKey, aResult);
}
