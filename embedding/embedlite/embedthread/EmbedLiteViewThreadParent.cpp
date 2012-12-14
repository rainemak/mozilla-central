/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EmbedLiteViewThreadParent.h"
#include "EmbedLog.h"
#include "EmbedLiteApp.h"

namespace mozilla {
namespace embedlite {

EmbedLiteViewThreadParent::EmbedLiteViewThreadParent(const uint32_t& id)
  : mId(id)
{
    MOZ_COUNT_CTOR(EmbedLiteViewThreadParent);
    LOGT("id:%u", mId);
    EmbedLiteApp::GetInstance()->RegisterViewImpl(this, id);
}

EmbedLiteViewThreadParent::~EmbedLiteViewThreadParent()
{
    MOZ_COUNT_DTOR(EmbedLiteViewThreadParent);
    LOGT();
}

void
EmbedLiteViewThreadParent::ActorDestroy(ActorDestroyReason aWhy)
{
    LOGT("reason:%i", aWhy);
}

} // namespace embedlite
} // namespace mozilla
