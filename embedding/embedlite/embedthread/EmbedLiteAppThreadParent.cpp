/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteAppThreadParent"

#include "EmbedLiteAppThreadParent.h"
#include "EmbedLiteApp.h"
#include "EmbedLog.h"

#include "mozilla/unused.h"

using namespace base;
using namespace mozilla::ipc;

namespace mozilla {
namespace embedlite {

static EmbedLiteAppThreadParent* sAppThreadParent = nullptr;

EmbedLiteAppThreadParent*
EmbedLiteAppThreadParent::GetAppThreadParent()
{
    return sAppThreadParent;
}

EmbedLiteAppThreadParent::EmbedLiteAppThreadParent(EmbedLiteApp* aApp)
  : mApp(aApp)
{
    MOZ_COUNT_CTOR(EmbedLiteAppThreadParent);
    sAppThreadParent = this;
}

EmbedLiteAppThreadParent::~EmbedLiteAppThreadParent()
{
    MOZ_COUNT_DTOR(EmbedLiteAppThreadParent);
    sAppThreadParent = nullptr;
}

void
EmbedLiteAppThreadParent::Stop()
{
    Close();
}

bool
EmbedLiteAppThreadParent::RecvStopped()
{
    return true;
}

bool
EmbedLiteAppThreadParent::Start()
{
    return true;
}

bool
EmbedLiteAppThreadParent::RecvInitialized()
{
    return true;
}

void
EmbedLiteAppThreadParent::ActorDestroy(ActorDestroyReason aWhy)
{
    LOGT("reason:%i", aWhy);
}

void EmbedLiteAppThreadParent::SetBoolPref(const char* aName, bool aValue)
{
    unused << SendSetBoolPref(nsCString(aName), aValue);
}

void EmbedLiteAppThreadParent::SetCharPref(const char* aName, const char* aValue)
{
    unused << SendSetCharPref(nsCString(aName), nsCString(aValue));
}

void EmbedLiteAppThreadParent::SetIntPref(const char* aName, int aValue)
{
    unused << SendSetIntPref(nsCString(aName), aValue);
}

} // namespace embedlite
} // namespace mozilla

