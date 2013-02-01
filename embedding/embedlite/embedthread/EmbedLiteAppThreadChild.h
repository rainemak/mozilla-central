/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_APP_EMBED_THREAD_CHILD_H
#define MOZ_APP_EMBED_THREAD_CHILD_H

#include "mozilla/embedlite/PEmbedLiteAppChild.h"
#include "EmbedLiteModulesService.h"

class EmbedLiteAppService;

namespace mozilla {
namespace embedlite {

class EmbedLiteAppThreadParent;
class EmbedLiteViewThreadChild;
class EmbedLiteAppThreadChild : public PEmbedLiteAppChild
{
    NS_INLINE_DECL_REFCOUNTING(EmbedLiteAppThreadChild)
public:
    EmbedLiteAppThreadChild(MessageLoop* aParentLoop);
    virtual ~EmbedLiteAppThreadChild();

    void Init(EmbedLiteAppThreadParent*);
    static EmbedLiteAppThreadChild* GetInstance();
    EmbedLiteModulesService* ModulesService() { return mModulesService; }
    EmbedLiteViewThreadChild* GetViewByID(uint32_t aId);
    EmbedLiteAppService* AppService();

protected:
    // Embed API ipdl interface
    virtual bool RecvSetBoolPref(const nsCString&, const bool&);
    virtual bool RecvSetCharPref(const nsCString&, const nsCString&);
    virtual bool RecvSetIntPref(const nsCString&, const int&);
    virtual bool RecvLoadGlobalStyleSheet(const nsCString&, const bool&);
    virtual bool RecvAsyncMessage(const nsString&, const nsString&);

    // IPDL protocol impl
    virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

    virtual bool RecvCreateView(const uint32_t&);
    virtual bool RecvPreDestroy();
    virtual PEmbedLiteViewChild* AllocPEmbedLiteView(const uint32_t&);
    virtual bool DeallocPEmbedLiteView(PEmbedLiteViewChild*);

private:
    void InitWindowWatcher();
    friend class EmbedLiteViewThreadChild;

    MessageLoop* mParentLoop;
    RefPtr<EmbedLiteAppThreadParent> mThreadParent;
    nsRefPtr<EmbedLiteModulesService> mModulesService;

    std::map<uint32_t, EmbedLiteViewThreadChild*> mWeakViewMap;

    DISALLOW_EVIL_CONSTRUCTORS(EmbedLiteAppThreadChild);
};

} // namespace embedlite
} // namespace mozilla

#endif // MOZ_APP_EMBED_THREAD_CHILD_H
