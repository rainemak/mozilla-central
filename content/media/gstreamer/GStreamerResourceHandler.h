/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef G_STREAMER_RESOURCE_HANDLER
#define G_STREAMER_RESOURCE_HANDLER

#include "mozilla/Mutex.h"

namespace mozilla {

class GStreamerResourceHandler
{
public:
    GStreamerResourceHandler();
    virtual ~GStreamerResourceHandler();
    bool IsWaitingMediaResources();
    bool IsDormantNeeded();
    void SetResource(void* resource);
private:
    void* mResourceSet;
    Mutex mLock;
};

} // namespace mozilla

#endif
