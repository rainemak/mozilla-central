/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=8 et :
 */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _EmbedKineticModule_H
#define _EmbedKineticModule_H

#include "mozilla/gfx/Point.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

class EmbedKineticListener
{
public:
    virtual ~EmbedKineticListener() {}
    virtual void ScrollViewBy(int x, int y) = 0;
    virtual void UpdateViewport() = 0;
    virtual void GetViewMargins(int& left, int& top, int& right, int& bottom) = 0;
    virtual void GetOffscreenViewMargins(int& left, int& top, int& right, int& bottom) = 0;
    virtual void GetViewSize(float& width, float& height) = 0;
    virtual void GetContentSize(float& width, float& height) = 0;
};

class EmbedKineticModule
{
public:
    EmbedKineticModule(EmbedKineticListener* aListener, mozilla::embedlite::EmbedLiteApp* aApp);
    virtual ~EmbedKineticModule();

    bool MouseMove(int x, int y, int mstime);
    void MousePress(int x, int y, int mstime);
    bool MouseRelease(int x, int y, int mstime);

private:
    void UpdateDirection(const mozilla::gfx::Point& aVector, bool aAllowOverride = false);
    void GetOverShoot(int &dx, int &dy);
    void LimitToOverShoots(double& dx, double& dy);
    void StopTimer();
    void EndOfMotion();
    void StartTimer();
    static void KineticMotionTimeoutStatic(void*);
    void KineticMotionTimeout();
    void SetOffscreenUpdateFrequency(int aXPercent, int aYPercent, int width, int height);
    void DisplayPortUpdated();
    // Returns int((a) * (b)) or at least 1 if a>0 (and respectively -1 if a<0)
    inline int mult_ceil(int a, float b);

    // Pointer velocity on X axis
    float mXSpeed;
    // Pointer velocity on Y axis
    float mYSpeed;
    int mPressTime;
    int mLastMoveTime;
    int mPressEventTime;
    mozilla::gfx::Point mPressPoint;
    mozilla::gfx::Point mLastPoint;
    mozilla::gfx::Point mReleaseDiff;
    mozilla::gfx::Point mLastDiff;
    mozilla::gfx::Point mAccScroll;
    mozilla::gfx::Point mLastMovePoint;
    enum PanAxis { UNKNOWN, VERTICAL, HORIZONTAL, ANY } mPanAxis;
    enum PanDirection { UNDEF, FORWARD, BACKWARD };
    PanDirection mPanDirectionX;
    PanDirection mPanDirectionY;
    bool mWaitingResponse;
    bool mUpdateViewPortOnStart;
    bool mEnabled;
    int mXUpdateDistance;
    int mYUpdateDistance;
    EmbedKineticListener* mListener;
    void* mKineticTask;
    mozilla::embedlite::EmbedLiteApp* mApp;
};

#endif // _EmbedKineticModule_H
