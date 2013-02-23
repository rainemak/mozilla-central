/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZ_APP_EMBED_THREAD_CHILD_H
#define MOZ_APP_EMBED_THREAD_CHILD_H

#include "mozilla/embedlite/PEmbedLiteAppChild.h"
#include "EmbedLiteModulesService.h"

class EmbedLiteAppService;
class nsIWebBrowserChrome;

namespace mozilla {
namespace embedlite {

class EmbedLiteAppThreadParent;
class EmbedLiteViewThreadChild;
class EmbedLiteAppThreadChild : public PEmbedLiteAppChild,
                                public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  EmbedLiteAppThreadChild(MessageLoop* aParentLoop);
  virtual ~EmbedLiteAppThreadChild();

  void Init(EmbedLiteAppThreadParent*);
  static EmbedLiteAppThreadChild* GetInstance();
  EmbedLiteModulesService* ModulesService() {
    return mModulesService;
  }
  EmbedLiteViewThreadChild* GetViewByID(uint32_t aId);
  ::EmbedLiteAppService* AppService();
  EmbedLiteViewThreadChild* GetViewByChromeParent(nsIWebBrowserChrome* aParent);

  virtual bool RecvCreateView(const uint32_t& id, const uint32_t& parentId);

protected:
  // Embed API ipdl interface
  virtual bool RecvSetBoolPref(const nsCString&, const bool&);
  virtual bool RecvSetCharPref(const nsCString&, const nsCString&);
  virtual bool RecvSetIntPref(const nsCString&, const int&);
  virtual bool RecvLoadGlobalStyleSheet(const nsCString&, const bool&);

  // IPDL protocol impl
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool RecvPreDestroy();
  virtual bool RecvObserve(const nsCString& topic,
                           const nsString& data);
  virtual bool RecvAddObserver(const nsCString&);
  virtual bool RecvRemoveObserver(const nsCString&);

  virtual PEmbedLiteViewChild* AllocPEmbedLiteView(const uint32_t&, const uint32_t& parentId);
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
