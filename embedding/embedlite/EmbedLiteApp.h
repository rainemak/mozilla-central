/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EMBED_LITE_APP_H
#define EMBED_LITE_APP_H

namespace mozilla {
namespace embedlite {

class EmbedLiteAppListener
{
public:
    virtual void Initialized() {};
    virtual void Destroyed() {};
};

class EmbedLiteApp
{
public:
    virtual ~EmbedLiteApp(); 

    enum EmbedType {
        EMBED_THREAD, // Initialize XPCOM in child thread
        EMBED_PROCESS // Initialize XPCOM in separate process
    };

    // Set Listener interface for EmbedLiteApp notifications
    virtual void SetListener(EmbedLiteAppListener* aListener);

    // Public Embedding API
    virtual bool Start(EmbedType aEmbedType);
    virtual void Stop();

    virtual void SetBoolPref(const char* aName, bool aValue);
    virtual void SetCharPref(const char* aName, const char* aValue);
    virtual void SetIntPref(const char* aName, int aValue);

    // Only one EmbedHelper object allowed
    static EmbedLiteApp* GetSingleton();
private:
    EmbedLiteApp();
    static EmbedLiteApp* sSingleton;
    EmbedLiteAppListener* mListener;
};

} // namespace embedlite
} // namespace mozilla

#endif // EMBED_LITE_APP_H
