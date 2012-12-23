/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=8 et :
 */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedKineticModule"

#include "EmbedKineticModule.h"
#include "EmbedLog.h"
#include <math.h>
#include <algorithm>

/// Limits (x) to values between (-m) and (m)
#define CAP(x, m) std::min(std::max((x), -(m)), (m))

#define KINETIC_SPEED 5000        // pixels/time^2 multiplier for initial kinetic motion speed, bigger is faster
#define KINETIC_SPEED_LIMIT 50.0f // maximum pixels/time kinetic motion speed
#define MINIMAL_MOVE 5            // px; pointer movement has to be this much or
// greater on either axis to be recorded
#define LOCK_SECTOR (M_PI/4)    // angle of the sector within which the motion is locked to horizontal/vertical, PI/4 = 45.
#define FPS 60                  // motion update frequency
#define ONESEC 1000             // One Second in milliseconds

#define KINETIC_DECAY 0.01      // bigger means motion stops faster, must be < 1
#define OVERSHOOT_DECAY 0.4     // bigger means overshoot is eliminated faster, must be < 1

#define MAX_MOVE_INTERVAL 1000  // maximum time between move events to consider them to belong to same gesture
#define MAX_MOVE_ANGLE (M_PI/3) // maximum angle difference between consecutive move events to consider them to belong to same gesture

static const int sKineticTimeout = ONESEC / FPS;
using namespace mozilla::embedlite;

EmbedKineticModule::EmbedKineticModule(EmbedKineticListener* aListener)
  : mXSpeed(0)
  , mYSpeed(0)
  , mPressTime(0)
  , mLastMoveTime(0)
  , mPressEventTime(0)
  , mPanAxis(UNKNOWN)
  , mPanDirectionX(UNDEF)
  , mPanDirectionY(UNDEF)
  , mWaitingResponse(false)
  , mUpdateViewPortOnStart(true)
  , mEnabled(true)
  , mXUpdateDistance(100)
  , mYUpdateDistance(100)
  , mListener(aListener)
  , mKineticTask(nullptr)
{
}

EmbedKineticModule::~EmbedKineticModule()
{
    StopTimer();
}

void EmbedKineticModule::UpdateDirection(const gfx::Point& aVector, bool aAllowOverride)
{
    if (((mPanAxis == UNKNOWN) || (aAllowOverride && mPanAxis == ANY)) &&
        (abs(aVector.x) > MINIMAL_MOVE || abs(aVector.y) > MINIMAL_MOVE)) {
        // check if movement vector differs at most 30% from X/Y axes
        if (atan2f(abs(aVector.y), abs(aVector.x)) < LOCK_SECTOR / 2) {
            mPanAxis = HORIZONTAL;
        }
        else if (atan2f(abs(aVector.x), abs(aVector.y)) < LOCK_SECTOR / 2) {
            mPanAxis = VERTICAL;
        }
        else {
            mPanAxis = ANY;
        }
    }
}

void EmbedKineticModule::GetOverShoot(int &dx, int &dy)
{
    int left = 0, top = 0, right = 0, bottom = 0;
    gfx::Size nsviewSize, nscontentSize;
//  mEmbed->GetViewMargins(left, top, right, bottom);
//  nsviewSize = mEmbed->nsGetViewSize(false);
//  nscontentSize = mEmbed->nsGetContentSize();
    bool smallwidth = nscontentSize.width < nsviewSize.width;
    bool smallheight = nscontentSize.height < nsviewSize.height;

    // the overshoot correction motion vector is the difference between both margins; this will
    // center the page if both left and right (top and bottom) margins exist
    dx = smallwidth && right > 0  ? 0 : right - left;
    dy = smallheight && bottom > 0 ? 0 : bottom - top;

    // If we reach a situation where the page would need only one pixel
    // compensation, but that compensation would then trigger another one
    // pixel compensation to the opposite direction, ad infinitum, then just
    // stop the motion when the page is aligned this one pixel to left.
    if (left >= 0 && right > 0 && dx == 1) dx = 0;
    // same thing for top/bottom
    if (top >= 0 && bottom > 0 && dy == 1) dy = 0;
}

void EmbedKineticModule::LimitToOverShoots(double& dx, double& dy)
{
    int left = 0, top = 0, right = 0, bottom = 0;
//  mEmbed->GetViewMargins(left, top, right, bottom, true);
    int maxOvershoot = 70;
    if ((dx > 0) && (dx - left > maxOvershoot)) dx = left + maxOvershoot;
    if ((dx < 0) && (dx + right < -maxOvershoot)) dx = -(right + maxOvershoot);
    if ((dy > 0) && (dy - top > maxOvershoot)) dy = top + maxOvershoot;
    if ((dy < 0) && (dy + bottom < -maxOvershoot)) dy = -(bottom + maxOvershoot);
}

bool EmbedKineticModule::MouseMove(int x, int y, int mstime)
{
    if (!mEnabled) {
        return false;
    }
    gfx::Point curPoint(x, y);
    gfx::Point d(curPoint - mLastPoint);
    mReleaseDiff = curPoint - mPressPoint;
    if ((abs(d.x) < MINIMAL_MOVE) && (abs(d.y) < MINIMAL_MOVE)) {
        // not great enough movement for us
        return true;
    }

    // flag to be set if movement should be considered as started again from this point
    bool resetStartPoint = (mstime - mLastMoveTime > MAX_MOVE_INTERVAL);
    gfx::Point lastPoint = mLastPoint;
    mLastPoint = curPoint;
    mLastMoveTime = mstime;

    // get overshoot margins
    int left = 0, top = 0, right = 0, bottom = 0;
//  mEmbed->GetViewMargins(left, top, right, bottom);
    int ox = right - left;
    int oy = bottom - top;

    // calculate requested motion difference, but limit it according to overshoot
    // if necessary
    double diffx = curPoint.x - lastPoint.x;
    double diffy = curPoint.y - lastPoint.y;

    // check if movement direction changed radically
    if (!resetStartPoint && (mLastDiff.x != 0 || mLastDiff.y != 0)) {
//    float angle = atan2f(diff.y, diff.x) - atan2f(mLastDiff.y, mLastDiff.x);
//    resetStartPoint = (fabs(angle) > MAX_MOVE_ANGLE);
    }
    mLastDiff = gfx::Point(diffx, diffy);

    // check if start point needs to be reset
    if (resetStartPoint) {
        // reset start point to make movement re-start from here
        mPressPoint = curPoint;
        mPressEventTime = mstime;
        mPanAxis = UNKNOWN;
    }

    // get current motion vector and check if we can decide the direction from it..
    gfx::Point vec(mLastPoint - mPressPoint);
    UpdateDirection(vec);
    if (mPanAxis == UNKNOWN) {
        return true;
    }

    // calculate maximum possible overshoot
    int maxOvershoot = 70;
    LimitToOverShoots(diffx, diffy);

    // set X difference to 0 if movement is limited to Y axis
    if (mPanAxis == VERTICAL) {
        diffx = 0;
        // only either left or right margin overshoot is visible: limit movement
        // according to the amount of allowed overshoot space
    } else if (ox != 0 && maxOvershoot) {
        if ((ox < 0 && diffx > 0) || (ox > 0 && diffx < 0)) {
            diffx = CAP(diffx, double(maxOvershoot)) * (maxOvershoot - std::min(std::abs(ox), maxOvershoot)) / maxOvershoot;
        }
    }

    // respective calculations for the Y difference
    if (mPanAxis == HORIZONTAL) {
        diffy = 0;
    } else if (oy != 0 && maxOvershoot) {
        if ((oy < 0 && diffy > 0) || (oy > 0 && diffy < 0)) {
            diffy = CAP(diffy, double(maxOvershoot)) * (maxOvershoot - abs(oy)) / maxOvershoot;
        }
    }

    // pan the document if possible
    if (diffx || diffy) {
        mListener->ScrollViewBy(diffx, diffy);
        if (mUpdateViewPortOnStart && mPanAxis) {
            mListener->UpdateViewport();
            mUpdateViewPortOnStart = false;
        }
    } else {
        mPanAxis = UNKNOWN;
    }

    // adjust the stored last point
    mLastPoint = lastPoint + gfx::Point(diffx, diffy);
    return true;
}

void EmbedKineticModule::MousePress(int x, int y, int mstime)
{
    if (!mEnabled) {
        return;
    }
    mReleaseDiff = gfx::Point();
    StopTimer();
    mPressTime = mLastMoveTime = mstime;
    mLastPoint = gfx::Point(x, y);
    mUpdateViewPortOnStart = true;
    mPressPoint = mLastPoint;
    mLastDiff = mReleaseDiff;
    mXSpeed = 0;
    mYSpeed = 0;
    mPanAxis = UNKNOWN;
}

bool EmbedKineticModule::MouseRelease(int x, int y, int mstime)
{
    if (!mEnabled) {
        return false;
    }
    gfx::Point finalVector(gfx::Point(x, y) - mPressPoint);
    UpdateDirection(finalVector, true);
    int motionTime = mstime - mPressTime;
    if (motionTime > 0) {
        mXSpeed = KINETIC_SPEED * finalVector.x / (motionTime * motionTime);
        mXSpeed = CAP(mXSpeed, KINETIC_SPEED_LIMIT);
        mYSpeed = KINETIC_SPEED * finalVector.y / (motionTime * motionTime);
        mYSpeed = CAP(mYSpeed, KINETIC_SPEED_LIMIT);
        if (mPanAxis == EmbedKineticModule::HORIZONTAL) {
            mYSpeed = 0;
            mPanDirectionX = mXSpeed < 0 ? EmbedKineticModule::FORWARD : EmbedKineticModule::BACKWARD;
            mPanDirectionY = EmbedKineticModule::UNDEF;
        }
        else if (mPanAxis == EmbedKineticModule::VERTICAL) {
            mXSpeed = 0;
            mPanDirectionY = mYSpeed < 0 ? EmbedKineticModule::FORWARD : EmbedKineticModule::BACKWARD;
            mPanDirectionX = EmbedKineticModule::UNDEF;
        } else {
            mPanDirectionY = mYSpeed < 0 ? EmbedKineticModule::FORWARD : EmbedKineticModule::BACKWARD;
            mPanDirectionX = mXSpeed < 0 ? EmbedKineticModule::FORWARD : EmbedKineticModule::BACKWARD;
        }
        LOGT("speed[%g,%g]", mXSpeed, mYSpeed);
    }
    mPressPoint = mAccScroll = gfx::Point(0, 0);
    mListener->UpdateViewport();
    StartTimer();
    return true;
}

//void EmbedKineticModule::timerEvent(QTimerEvent*e)
//{
//  KineticMotionTimeout();
//}

void EmbedKineticModule::StopTimer()
{
    LOGT();
    if (mKineticTask) {
        mKineticTask->Cancel();
        mKineticTask = nullptr;
    }
}

void EmbedKineticModule::EndOfMotion()
{
    StopTimer();
    mXSpeed = 0;
    mYSpeed = 0;
    mPanAxis = UNKNOWN;
    mListener->UpdateViewport();
}

void EmbedKineticModule::StartTimer()
{
    StopTimer();
    LOGT("sKineticTimeout:%i", sKineticTimeout);
    mKineticTask =
        NewRunnableMethod(this,
                          &EmbedKineticModule::KineticMotionTimeout);
    MessageLoop::current()->
        PostDelayedTask(FROM_HERE, mKineticTask, sKineticTimeout);
}

void EmbedKineticModule::KineticMotionTimeout()
{
    LOGT();
    float& xs = mXSpeed;
    float& ys = mYSpeed;

    // compensate overshoot
    int ox = 0, oy = 0;
    GetOverShoot(ox, oy);
    int panx = 0, pany = 0;
    if (ox == 0) { // pan according to kinetic motion
        panx = int(roundf(xs));
        xs = int((1.0 - KINETIC_DECAY) * xs);
    }
    else { // pan according to overshoot
        panx = mult_ceil(ox, OVERSHOOT_DECAY);
        xs = 0;
    }
    if (oy == 0) {
        pany = int(roundf(ys));
        ys = int((1.0 - KINETIC_DECAY) * ys);
    }
    else {
        pany = mult_ceil(oy, OVERSHOOT_DECAY);
        ys = 0;
    }

    double panxd = panx, panyd = pany;
    LimitToOverShoots(panxd, panyd);
    panx = round(panxd);
    pany = round(panyd);

    // check if we are out of momentum
    if (panx == 0 && pany == 0) {
        EndOfMotion();
        return;
    }
    mListener->ScrollViewBy(panx, pany);
    mAccScroll += gfx::Point(abs(panx), abs(pany));
    if (mAccScroll.x > mXUpdateDistance ||
        mAccScroll.y > mYUpdateDistance) {
        mAccScroll = gfx::Point(0, 0);
        mWaitingResponse = true;
        mListener->UpdateViewport();
    }
    StartTimer();
}

void EmbedKineticModule::SetOffscreenUpdateFrequency(int aXPercent, int aYPercent, int width, int height)
{
    mXUpdateDistance = width / 100 * aXPercent;
    mYUpdateDistance = height / 100 * aYPercent;
}

void EmbedKineticModule::DisplayPortUpdated()
{
    mWaitingResponse = false;
}

// Returns int((a) * (b)) or at least 1 if a>0 (and respectively -1 if a<0)
int EmbedKineticModule::mult_ceil(int a, float b)
{
    int tmp = int(roundf(a * b));
    if (tmp == 0) {
        if (a > 0) return 1;
        if (a < 0) return -1;
    }
    return tmp;
}
