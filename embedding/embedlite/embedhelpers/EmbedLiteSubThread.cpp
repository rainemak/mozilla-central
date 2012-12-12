/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=8 et :
 */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteThread"

#include "EmbedLiteSubThread.h"
#include "GeckoLoader.h"
#include "EmbedLog.h"
#include "EmbedLiteApp.h"

namespace mozilla {
namespace embedlite {

//
// EmbedLiteSubThread
//

EmbedLiteSubThread::EmbedLiteSubThread(EmbedLiteApp* aApp)
  : base::Thread("EmbedLiteSubThread")
  , mParentLoop(MessageLoop::current())
  , mApp(aApp)
{
    LOGT();
}

EmbedLiteSubThread::~EmbedLiteSubThread()
{
    LOGT();
}

void EmbedLiteSubThread::PostToParent()
{
    if (mApp->GetListener()) {
        mApp->GetListener()->Initialized();
    }
}

void EmbedLiteSubThread::Init()
{
    LOGT();
    GeckoLoader::InitEmbedding("mozembed");
    mParentLoop->PostTask(FROM_HERE,
                          NewRunnableMethod(this,
                                            &EmbedLiteSubThread::PostToParent));
}

void EmbedLiteSubThread::CleanUp()
{
    LOGT();
    GeckoLoader::TermEmbedding();
}

bool EmbedLiteSubThread::StartEmbedThread()
{
    LOGT();
    return StartWithOptions(Thread::Options(MessageLoop::TYPE_MOZILLA_CHILD, 0));
}

} // namespace embedlite
} // namespace mozilla
