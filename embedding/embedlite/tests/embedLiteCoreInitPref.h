/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/embedlite/EmbedLiteApp.h"

using namespace mozilla::embedlite;

void InitDefaultEmbedBrowserPrefs(EmbedLiteApp* aApp)
{
    aApp->SetBoolPref("plugin.disable", true);
    aApp->SetBoolPref("dom.ipc.plugins.enabled", true);
    aApp->SetCharPref("plugins.force.wmode", "opaque");
    aApp->SetBoolPref("browser.xul.error_pages.enabled", true);
    aApp->SetIntPref("gfx.color_management.mode", 0);
    aApp->SetCharPref("general.useragent.override", "Mozilla/5.0 (Maemo; Linux armv7l; rv:14.0) Gecko/17.0 Firefox/17.0");
    aApp->SetBoolPref("nglayout.debug.paint_flashing", getenv("FLASH_PAINT") != 0);
    // Perf trick, speedup motion handlers
    aApp->SetBoolPref("layout.reflow.synthMouseMove", false);

    // Disable Native themeing because it is usually broken
    aApp->SetBoolPref("mozilla.widget.disable-native-theme", true);

    // Override some named colors to avoid inverse OS themes
    aApp->SetCharPref("ui.-moz-dialog", "#efebe7");
    aApp->SetCharPref("ui.-moz-dialogtext", "#101010");
    aApp->SetCharPref("ui.-moz-field", "#fff");
    aApp->SetCharPref("ui.-moz-fieldtext", "#1a1a1a");
    aApp->SetCharPref("ui.-moz-buttonhoverface", "#f3f0ed");
    aApp->SetCharPref("ui.-moz-buttonhovertext", "#101010");
    aApp->SetCharPref("ui.-moz-combobox", "#fff");
    aApp->SetCharPref("ui.-moz-comboboxtext", "#101010");
    aApp->SetCharPref("ui.buttonface", "#ece7e2");
    aApp->SetCharPref("ui.buttonhighlight", "#fff");
    aApp->SetCharPref("ui.buttonshadow", "#aea194");
    aApp->SetCharPref("ui.buttontext", "#101010");
    aApp->SetCharPref("ui.captiontext", "#101010");
    aApp->SetCharPref("ui.graytext", "#b1a598");
    aApp->SetCharPref("ui.highlight", "#fad184");
    aApp->SetCharPref("ui.highlighttext", "#1a1a1a");
    aApp->SetCharPref("ui.infobackground", "#f5f5b5");
    aApp->SetCharPref("ui.infotext", "#000");
    aApp->SetCharPref("ui.menu", "#f7f5f3");
    aApp->SetCharPref("ui.menutext", "#101010");
    aApp->SetCharPref("ui.threeddarkshadow", "#000");
    aApp->SetCharPref("ui.threedface", "#ece7e2");
    aApp->SetCharPref("ui.threedhighlight", "#fff");
    aApp->SetCharPref("ui.threedlightshadow", "#ece7e2");
    aApp->SetCharPref("ui.threedshadow", "#aea194");
    aApp->SetCharPref("ui.window", "#efebe7");
    aApp->SetCharPref("ui.windowtext", "#101010");
    aApp->SetCharPref("ui.windowframe", "#efebe7");

    // Enable webgl by force
    aApp->SetBoolPref("webgl.force-enabled", true);
    // Setup dumping enabled for development
    aApp->SetBoolPref("browser.dom.window.dump.enabled", true);
    aApp->SetBoolPref("layers.acceleration.draw-fps", true);
    // SetPref(QString("browser.xul.error_pages.enabled", false);
    // Bug 706179 async animation temporary prefs
    aApp->SetBoolPref("layers.offmainthreadcomposition.animate-opacity", true);
    aApp->SetBoolPref("layers.offmainthreadcomposition.animate-transform", true);
    aApp->SetBoolPref("layers.async-video.enabled", true);
}
