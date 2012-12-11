/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EMBED_INIT_GLUE_H
#define EMBED_INIT_GLUE_H

#include <stdio.h>
#include "mozilla/embedlite/EmbedLiteAPI.h"

#include "nscore.h"

// getenv
#include <stdlib.h>
#include <iostream>

// Possibly only UNIX
#include <unistd.h>

// XPCOMGlueStartup
#include "nsXPCOMGlue.h"

// XRE_ Functions
#include "nsXULAppAPI.h"

#if defined(XP_WIN)
#include <windows.h>
#include <stdlib.h>
#elif defined(XP_UNIX)
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef XP_MACOSX
#include "MacQuirks.h"
#endif

#include "mozilla/Telemetry.h"

#ifdef XRE_HAS_DLL_BLOCKLIST
XRE_SetupDllBlocklistType XRE_SetupDllBlocklist = 0;
#endif
XRE_TelemetryAccumulateType XRE_TelemetryAccumulate = 0;
XRE_GetEmbedLiteType XRE_GetEmbedLite = 0;
XRE_InitEmbedding2Type XRE_InitEmbedding2 = 0;
XRE_TermEmbeddingType XRE_TermEmbedding = 0;
XRE_NotifyProfileType XRE_NotifyProfile = 0;
XRE_LockProfileDirectoryType XRE_LockProfileDirectory = 0;
XRE_AddManifestLocationType XRE_AddManifestLocation = 0;
XRE_GetBinaryPathType XRE_GetBinaryPath = 0;

static inline bool IsLibXulInThePath(const char* path, std::string& xpcomPath)
{
    xpcomPath = path;
    xpcomPath += "/libxpcom.so";
    struct stat buf;
    return !stat(xpcomPath.c_str(), &buf);
}

bool LoadEmbedLite(int argc = 0, char** argv = 0)
{
    if (XRE_TelemetryAccumulate) {
        printf("EmbedLite already loaded\n");
        return false;
    }
    nsresult rv;
    // find xpcom shared lib (uses GRE_HOME env var if set, current DIR, or executable binary path)
    std::string xpcomPath;
    char temp[MAX_PATH];
    const char* greHome = getenv("GRE_HOME");
    M_TRACE("greHome:%s", greHome);
    if (!greHome) {
        greHome = getenv("PWD");
    }
    printf("greHome:%s", greHome);
    if (!greHome) {
        printf("GRE_HOME is not defined\n");
        return NSR_TO_BOOL(NS_ERROR_FAILURE);
    }
    if (!IsLibXulInThePath(greHome, xpcomPath) && argv && argc) {
        printf("libxul.so not in gre home or PWD:%s, check in executable path\n", greHome);
        char *lastslash = strrchr(argv[0], '/');
        size_t path_size = &lastslash[0] - argv[0];
        strncpy(temp, argv[0], path_size);
        temp[path_size] = 0;
        greHome = &temp[0];
        if (!IsLibXulInThePath(temp, xpcomPath)) {
            printf("libxul.so is not found, in %s return fail\n", xpcomPath.c_str());
            return false;
        }
    }
    setenv("GRE_HOME", greHome, 1);
    setenv("MOZILLA_FIVE_HOME", greHome, 1);
    setenv("XRE_LIBXPCOM_PATH", xpcomPath.c_str(), 1);

#ifdef XP_MACOSX
    TriggerQuirks();
#endif

    NS_LogInit();

    int gotCounters;
#if defined(XP_UNIX)
    struct rusage initialRUsage;
    gotCounters = !getrusage(RUSAGE_SELF, &initialRUsage);
#elif defined(XP_WIN)
    // GetProcessIoCounters().ReadOperationCount seems to have little to
    // do with actual read operations. It reports 0 or 1 at this stage
    // in the program. Luckily 1 coincides with when prefetch is
    // enabled. If Windows prefetch didn't happen we can do our own
    // faster dll preloading.
    IO_COUNTERS ioCounters;
    gotCounters = GetProcessIoCounters(GetCurrentProcess(), &ioCounters);
    if (gotCounters && !ioCounters.ReadOperationCount)
#endif
    {
        XPCOMGlueEnablePreload();
    }

    // start the glue, i.e. load and link against xpcom shared lib
    rv = XPCOMGlueStartup(xpcomPath.c_str());
    if (NS_FAILED(rv)) {
        printf("Could not start XPCOM glue.\n");
        return NSR_TO_BOOL(NS_ERROR_FAILURE);
    }

    // load XUL functions
    nsDynamicFunctionLoad nsFuncs[] = {
        { "XRE_GetEmbedLite", (NSFuncPtr*) &XRE_GetEmbedLite },
#ifdef XRE_HAS_DLL_BLOCKLIST
        { "XRE_SetupDllBlocklist", (NSFuncPtr*) &XRE_SetupDllBlocklist },
#endif
        { "XRE_TelemetryAccumulate", (NSFuncPtr*) &XRE_TelemetryAccumulate },
        { "XRE_InitEmbedding2", (NSFuncPtr*)&XRE_InitEmbedding2},
        { "XRE_TermEmbedding", (NSFuncPtr*)&XRE_TermEmbedding},
        { "XRE_NotifyProfile", (NSFuncPtr*)&XRE_NotifyProfile},
        { "XRE_LockProfileDirectory", (NSFuncPtr*)&XRE_LockProfileDirectory},
        { "XRE_AddManifestLocation", (NSFuncPtr*)&XRE_AddManifestLocation},
        { "XRE_GetBinaryPath", (NSFuncPtr*)&XRE_GetBinaryPath},
        {0, 0}
    };

    rv = XPCOMGlueLoadXULFunctions(nsFuncs);
    if (NS_FAILED(rv)) {
        printf("Could not load XUL functions.\n");
        return NSR_TO_BOOL(NS_ERROR_FAILURE);
    }

#ifdef XRE_HAS_DLL_BLOCKLIST
    XRE_SetupDllBlocklist();
#endif

    if (gotCounters) {
#if defined(XP_WIN)
        XRE_TelemetryAccumulate(mozilla::Telemetry::EARLY_GLUESTARTUP_READ_OPS,
                                int(ioCounters.ReadOperationCount));
        XRE_TelemetryAccumulate(mozilla::Telemetry::EARLY_GLUESTARTUP_READ_TRANSFER,
                                int(ioCounters.ReadTransferCount / 1024));
        IO_COUNTERS newIoCounters;
        if (GetProcessIoCounters(GetCurrentProcess(), &newIoCounters)) {
            XRE_TelemetryAccumulate(mozilla::Telemetry::GLUESTARTUP_READ_OPS,
                                    int(newIoCounters.ReadOperationCount - ioCounters.ReadOperationCount));
            XRE_TelemetryAccumulate(mozilla::Telemetry::GLUESTARTUP_READ_TRANSFER,
                                    int((newIoCounters.ReadTransferCount - ioCounters.ReadTransferCount) / 1024));
        }
#elif defined(XP_UNIX)
        XRE_TelemetryAccumulate(mozilla::Telemetry::EARLY_GLUESTARTUP_HARD_FAULTS,
                                int(initialRUsage.ru_majflt));
        struct rusage newRUsage;
        if (!getrusage(RUSAGE_SELF, &newRUsage)) {
            XRE_TelemetryAccumulate(mozilla::Telemetry::GLUESTARTUP_HARD_FAULTS,
                                    int(newRUsage.ru_majflt - initialRUsage.ru_majflt));
        }
#endif
    }

    printf("XPCOM FUNCTIONS SUCCESEFULLY LOADED");

    return true;
}

void UnloadEmbedLite()
{
    XPCOMGlueShutdown();
}

#endif // EMBED_INIT_GLUE_H
