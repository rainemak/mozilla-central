/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/embedlite/EmbedInitGlue.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

#ifdef MOZ_WIDGET_QT
#include <QApplication>
#elif defined(MOZ_WIDGET_GTK2)
#include <glib/glib.h>
#endif

using namespace mozilla::embedlite;

static bool sDoExit = getenv("NORMAL_EXIT");

class MyListener : public EmbedLiteAppListener
{
public:
    MyListener(EmbedLiteApp* aApp) : mApp(aApp) {}
    virtual ~MyListener()
    {
    }
    virtual void Initialized()
    {
        printf("Embedding initialized, let's make view");
        mApp->SetBoolPref("plugin.disable", true);
        mApp->SetBoolPref("dom.ipc.plugins.enabled", true);
        mApp->SetCharPref("plugins.force.wmode", "opaque");
        mApp->SetBoolPref("browser.xul.error_pages.enabled", true);
        mApp->SetIntPref("gfx.color_management.mode", 0);
        mApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (Maemo; Linux armv7l; rv:14.0) Gecko/17.0 Firefox/17.0");
        mApp->SetBoolPref("nglayout.debug.paint_flashing", getenv("FLASH_PAINT") != 0);
        // Perf trick, speedup motion handlers
        mApp->SetBoolPref("layout.reflow.synthMouseMove", false);

        // Disable Native themeing because it is usually broken
        mApp->SetBoolPref("mozilla.widget.disable-native-theme", true);

        // Override some named colors to avoid inverse OS themes
        mApp->SetCharPref("ui.-moz-dialog", "#efebe7");
        mApp->SetCharPref("ui.-moz-dialogtext", "#101010");
        mApp->SetCharPref("ui.-moz-field", "#fff");
        mApp->SetCharPref("ui.-moz-fieldtext", "#1a1a1a");
        mApp->SetCharPref("ui.-moz-buttonhoverface", "#f3f0ed");
        mApp->SetCharPref("ui.-moz-buttonhovertext", "#101010");
        mApp->SetCharPref("ui.-moz-combobox", "#fff");
        mApp->SetCharPref("ui.-moz-comboboxtext", "#101010");
        mApp->SetCharPref("ui.buttonface", "#ece7e2");
        mApp->SetCharPref("ui.buttonhighlight", "#fff");
        mApp->SetCharPref("ui.buttonshadow", "#aea194");
        mApp->SetCharPref("ui.buttontext", "#101010");
        mApp->SetCharPref("ui.captiontext", "#101010");
        mApp->SetCharPref("ui.graytext", "#b1a598");
        mApp->SetCharPref("ui.highlight", "#fad184");
        mApp->SetCharPref("ui.highlighttext", "#1a1a1a");
        mApp->SetCharPref("ui.infobackground", "#f5f5b5");
        mApp->SetCharPref("ui.infotext", "#000");
        mApp->SetCharPref("ui.menu", "#f7f5f3");
        mApp->SetCharPref("ui.menutext", "#101010");
        mApp->SetCharPref("ui.threeddarkshadow", "#000");
        mApp->SetCharPref("ui.threedface", "#ece7e2");
        mApp->SetCharPref("ui.threedhighlight", "#fff");
        mApp->SetCharPref("ui.threedlightshadow", "#ece7e2");
        mApp->SetCharPref("ui.threedshadow", "#aea194");
        mApp->SetCharPref("ui.window", "#efebe7");
        mApp->SetCharPref("ui.windowtext", "#101010");
        mApp->SetCharPref("ui.windowframe", "#efebe7");

        // Enable webgl by force
        mApp->SetBoolPref("webgl.force-enabled", true);
        // Setup dumping enabled for development
        mApp->SetBoolPref("browser.dom.window.dump.enabled", true);
        mApp->SetBoolPref("layers.acceleration.draw-fps", true);
        // SetPref(QString("browser.xul.error_pages.enabled", false);
        // Bug 706179 async animation temporary prefs
        mApp->SetBoolPref("layers.offmainthreadcomposition.animate-opacity", true);
        mApp->SetBoolPref("layers.offmainthreadcomposition.animate-transform", true);
        mApp->SetBoolPref("layers.async-video.enabled", true);
        mApp->Stop();
    };
    bool ExecuteChildThread()
    {
        printf("Embedding ExecuteChildThread, return false\n");
        return false;
    };


private:
    EmbedLiteApp* mApp;
};

int main(int argc, char** argv)
{
#ifdef MOZ_WIDGET_QT
    QApplication app(argc, argv);
#elif defined(MOZ_WIDGET_GTK2)
    g_thread_init(NULL);
#endif

    printf("Load XUL Symbols\n");
    if (LoadEmbedLite(argc, argv)) {
        printf("XUL Symbols loaded\n");
        EmbedLiteApp* mapp = XRE_GetEmbedLite();
        MyListener* listener = new MyListener(mapp);
        mapp->SetListener(listener);
        bool res = mapp->Start(EmbedLiteApp::EMBED_THREAD);
        printf("XUL Symbols loaded: init res:%i\n", res);
        delete listener;
        delete mapp;
    } else {
        printf("XUL Symbols failed to load\n");
    }
    UnloadEmbedLite();
    return 0;
}
