/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EmbedLiteViewThreadChild.h"
#include "EmbedLog.h"

namespace mozilla {
namespace embedlite {

EmbedLiteViewThreadChild::EmbedLiteViewThreadChild()
{
    LOGT();
}

EmbedLiteViewThreadChild::~EmbedLiteViewThreadChild()
{
    LOGT();
}

void
EmbedLiteViewThreadChild::ActorDestroy(ActorDestroyReason aWhy)
{
    LOGT("reason:%i", aWhy);
}

bool EmbedLiteViewThreadChild::RecvDestroy()
{
    LOGT("destroy");
    PEmbedLiteViewChild::Send__delete__(this);
    return true;
}

} // namespace embedlite
} // namespace mozilla

