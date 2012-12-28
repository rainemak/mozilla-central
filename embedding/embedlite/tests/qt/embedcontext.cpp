/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "embedcontext.h"
#include <QEventLoop>
#include <QTimer>
#include "mozilla/embedlite/EmbedInitGlue.h"

using namespace mozilla::embedlite;

void
GeckoThread::Quit()
{
    if (mEventLoop)
        mEventLoop->quit();
    quit();
    wait();
}

void
GeckoThread::run()
{
    mContext->mApp->StartChildThread();
    mEventLoop = new QEventLoop();
    mEventLoop->exec();
    printf("Call Term StopChildThread\n");
    mContext->mApp->StopChildThread();
    delete mEventLoop;
    mEventLoop = 0;
}


EmbedContext::EmbedContext(int argc, char *argv[], QObject*)
  : mThread(new GeckoThread(this))
  , mInitialized(false)
{
    LoadEmbedLite(argc, argv);
    mApp = XRE_GetEmbedLite();
    mApp->SetListener(this);
}

EmbedContext::~EmbedContext()
{
    delete mThread;
    UnloadEmbedLite();
}

void
EmbedContext::runEmbedding()
{
    mApp->Start(EmbedLiteApp::EMBED_THREAD);
}

void
EmbedContext::Quit()
{
    GetApp()->Stop();
}

void
EmbedContext::Init()
{
    QTimer::singleShot(0, this, SLOT(runEmbedding()));
}

void EmbedContext::onLastWindowClosed()
{
    printf("EmbedContext::onLastWindowClosed\n");
    Quit();
}

void EmbedContext::contextInitialized()
{
    printf("EmbedContext moz embed initialized\n");
    Initialized();
}

void
EmbedContext::setDefaultPrefs()
{
    mApp->SetBoolPref("plugin.disable", true);
    mApp->SetBoolPref("dom.ipc.plugins.enabled", true);
    mApp->SetCharPref("plugins.force.wmode", "opaque");
    mApp->SetBoolPref("browser.xul.error_pages.enabled", true);
    mApp->SetIntPref("gfx.color_management.mode", 0);
    mApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (Maemo; Linux armv7l; rv:14.0) Gecko/17.0 Firefox/17.0");
    mApp->SetBoolPref("nglayout.debug.paint_flashing", getenv("FLASH_PAINT") != 0);
    mApp->SetBoolPref("nglayout.debug.widget_update_flashing", getenv("FLASH_PAINT_WGT") != 0);
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
}

// EmbedLiteAppListener iface

bool EmbedContext::StopChildThread()
{
    if (mThread) {
        mThread->Quit();
        return true;
    }

    return false;
}

bool EmbedContext::ExecuteChildThread()
{
    if (!getenv("GECKO_THREAD")) {
        connect(this, SIGNAL(contextFinalizedThread()), mThread, SLOT(Quit()));
        mThread->start();
        return true;
    }
    return false;
}

void EmbedContext::Initialized()
{
    printf("EmbedContext moz embed initialized2\n");
    // TODO Fix pending prefs init in embedApp
    mInitialized = true;
    setDefaultPrefs();
    GetApp()->LoadGlobalStyleSheet("chrome://global/content/embedScrollStyles.css", true);
    emit geckoContextInitialized();
}

void EmbedContext::Destroyed()
{
    emit contextFinalized();
}
