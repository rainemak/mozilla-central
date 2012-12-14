/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EMBED_LITE_APP_H
#define EMBED_LITE_APP_H

#include "mozilla/RefPtr.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteUILoop;
class EmbedLiteSubThread;
class EmbedLiteAppThread;
class EmbedLiteAppListener
{
public:
    virtual bool ExecuteChildThread() { return false; };
    virtual void Initialized() {};
    virtual void Destroyed() {};
};

class EmbedLiteApp
{
public:
    virtual ~EmbedLiteApp(); 

    enum EmbedType {
        EMBED_INVALID,// Default value 
        EMBED_THREAD, // Initialize XPCOM in child thread
        EMBED_PROCESS // Initialize XPCOM in separate process
    };

    // Set Listener interface for EmbedLiteApp notifications
    virtual void SetListener(EmbedLiteAppListener* aListener);
    

    // Public Embedding API

    // Start UI embedding loop merged with Gecko GFX, blocking call until Stop() called
    virtual bool Start(EmbedType aEmbedType);
    // Exit from UI embedding loop started with Start()
    virtual void Stop();

    // This must be called in native toolkit child thread, only after ExecuteChildThread call
    virtual bool StartChildThread();
    // Must be called from same thread as StartChildThread, and before Stop()
    virtual bool StopChildThread();

    // Setup preferences
    virtual void SetBoolPref(const char* aName, bool aValue);
    virtual void SetCharPref(const char* aName, const char* aValue);
    virtual void SetIntPref(const char* aName, int aValue);

    // Internal
    EmbedLiteAppListener* GetListener() { return mListener; }

    // Only one EmbedHelper object allowed
    static EmbedLiteApp* GetInstance();
private:
    static void StartChild(EmbedLiteApp* aApp);

    EmbedLiteApp();
    static EmbedLiteApp* sSingleton;
    EmbedLiteAppListener* mListener;
    EmbedLiteUILoop* mUILoop;
    RefPtr<EmbedLiteSubThread> mSubThread;
    EmbedType mEmbedType;
    RefPtr<EmbedLiteAppThread> mAppThread;
};

} // namespace embedlite
} // namespace mozilla

#endif // EMBED_LITE_APP_H
