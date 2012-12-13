/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_APP_EMBED_THREAD_PARENT_H
#define MOZ_APP_EMBED_THREAD_PARENT_H

#include "mozilla/embedlite/PEmbedLiteAppParent.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteApp;
class EmbedLiteAppThreadParent : public PEmbedLiteAppParent
{
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(EmbedLiteAppThreadParent)
public:
    EmbedLiteAppThreadParent(EmbedLiteApp* aApp);
    virtual ~EmbedLiteAppThreadParent();

    virtual bool Start();
    virtual void Stop();
    virtual void SetBoolPref(const char* aName, bool aValue);
    virtual void SetCharPref(const char* aName, const char* aValue);
    virtual void SetIntPref(const char* aName, int aValue);

    // IPDL
    virtual bool
    RecvStopped();

    virtual bool
    RecvInitialized();

    /**
     * Returns the compositor thread's message loop.
     *
     * This message loop is used by CompositorParent and ImageBridgeParent.
     */
    MessageLoop* EmbedLiteAppLoop();

    static EmbedLiteAppThreadParent* GetAppThreadParent();

protected:
    virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

private:
    EmbedLiteApp* mApp;

private:
    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteAppThreadParent);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_APP_EMBED_THREAD_PARENT_H
