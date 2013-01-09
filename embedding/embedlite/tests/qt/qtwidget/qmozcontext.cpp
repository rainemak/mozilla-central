/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QTimer>
#include <QApplication>

#include "qmozcontext.h"

#include "nsDebug.h"
#include "mozilla/embedlite/EmbedLog.h"
#include "mozilla/embedlite/EmbedLiteApp.h"
#include "mozilla/embedlite/EmbedInitGlue.h"

using namespace mozilla::embedlite;

static QMozContext* sSingleton = nullptr;

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
    mContext->GetApp()->StartChildThread();
    mEventLoop = new QEventLoop();
    mEventLoop->exec();
    printf("Call Term StopChildThread\n");
    mContext->GetApp()->StopChildThread();
    delete mEventLoop;
    mEventLoop = 0;
}

class QMozContextPrivate : public EmbedLiteAppListener {
public:
    QMozContextPrivate(QMozContext* qq)
    : q(qq)
    , mApp(NULL)
    , mInitialized(false)
    , mThread(new GeckoThread(qq))
    {
    }
    virtual ~QMozContextPrivate() {
        delete mThread;
    }

    virtual bool ExecuteChildThread() {
        LOGT();
        if (!getenv("GECKO_THREAD")) {
            mThread->start();
            mThread->setPriority(QThread::LowPriority);
            return true;
        }
        return false;
    }
    // Native thread must be stopped here
    virtual bool StopChildThread() {
        LOGT();
        if (mThread) {
            mThread->Quit();
            return true;
        }
        return false;
    }
    // App Initialized and ready to API call
    virtual void Initialized() {
        mInitialized = true;
        setDefaultPrefs();
        mApp->LoadGlobalStyleSheet("chrome://global/content/embedScrollStyles.css", true);
        q->emit onInitialized();
    }
    // App Destroyed, and ready to delete and program exit
    virtual void Destroyed() {
        LOGT();
    }

    void setDefaultPrefs()
    {
        LOGT();
        mApp->SetBoolPref("plugin.disable", true);
        mApp->SetBoolPref("dom.ipc.plugins.enabled", true);
        mApp->SetCharPref("plugins.force.wmode", "opaque");
        mApp->SetBoolPref("browser.xul.error_pages.enabled", true);
        mApp->SetIntPref("gfx.color_management.mode", 0);
//        mApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (Maemo; Linux armv7l; rv:14.0) Gecko/17.0 Firefox/17.0");
        mApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (Android; Mobile; rv:12.0) Gecko/17.0 Firefox/17.0");
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
        mApp->SetBoolPref("layers.acceleration.draw-fps", getenv("SHOW_FPS") != 0);
        // SetPref(QString("browser.xul.error_pages.enabled", false);
        // Bug 706179 async animation temporary prefs
        mApp->SetBoolPref("layers.offmainthreadcomposition.animate-opacity", true);
        mApp->SetBoolPref("layers.offmainthreadcomposition.animate-transform", true);
        mApp->SetBoolPref("layers.async-video.enabled", true);
        mApp->SetBoolPref("font.size.inflation.disabledInMasterProcess", true);
        mApp->SetIntPref("gfx.azpc.pan_repaint_interval", 5050);
        mApp->SetCharPref("gfx.azpc.y_skate_size_multiplier", "4.5f");
        mApp->SetCharPref("gfx.azpc.y_stationary_size_multiplier", "4.5f");
        mApp->SetCharPref("gfx.axis.max_event_acceleration", "12.0f");
        mApp->SetCharPref("gfx.axis.fling_friction", "0.00245f");
        mApp->SetCharPref("gfx.azpc.min_skate_speed", "10.0f");
        mApp->SetBoolPref("embedlite.handle_viewport", getenv("USE_VIEWPORT") != 0);
        mApp->SetIntPref("ui.dragThresholdX", 25);
        mApp->SetIntPref("ui.dragThresholdY", 25);
        mApp->SetBoolPref("layout.build_layers_for_scrollable_views", true);
    }

private:
    QMozContext* q;
    EmbedLiteApp* mApp;
    bool mInitialized;
    friend class QMozContext;
    friend class GeckoThread;
    GeckoThread* mThread;
};

QMozContext::QMozContext(QObject* parent)
    : QObject(parent)
    , d(new QMozContextPrivate(this))
{
    LoadEmbedLite();
    d->mApp = XRE_GetEmbedLite();
    d->mApp->SetListener(d);
    QObject::connect(qApp, SIGNAL(lastWindowClosed()), this, SLOT(onLastWindowClosed()));
    QTimer::singleShot(0, this, SLOT(runEmbedding()));
}

QMozContext::~QMozContext()
{
    UnloadEmbedLite();
    delete d;
}

QMozContext*
QMozContext::GetInstance()
{
    if (!sSingleton) {
        sSingleton = new QMozContext();
        NS_ASSERTION(sSingleton, "not initialized");
    }
    return sSingleton;
}

void QMozContext::runEmbedding()
{
    d->mApp->Start(EmbedLiteApp::EMBED_THREAD);
}

bool
QMozContext::initialized()
{
    return d->mInitialized;
}

EmbedLiteApp*
QMozContext::GetApp()
{
    return d->mApp;
}

void QMozContext::onLastWindowClosed()
{
    GetApp()->Stop();
}
