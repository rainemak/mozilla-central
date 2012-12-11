/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EMBED_LITE_APP_H
#define EMBED_LITE_APP_H

namespace mozilla {
namespace embedlite {

class EmbedLiteApp
{
public:
    virtual ~EmbedLiteApp();

    enum EmbedType {
        EMBED_THREAD, // Initialize XPCOM in child thread
        EMBED_PROCESS // Initialize XPCOM in separate process
    };

    // Only one EmbedHelper object allowed
    static EmbedLiteApp* GetSingleton();

private:
    EmbedLiteApp();

    static EmbedLiteApp* sSingleton;
};

} // namespace embedlite
} // namespace mozilla

#endif // EMBED_LITE_APP_H
