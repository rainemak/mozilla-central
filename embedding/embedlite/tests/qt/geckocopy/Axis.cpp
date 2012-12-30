/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=4 ts=8 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Axis.h"
#include "AsyncPanZoomController.h"

using namespace mozilla::layers;

namespace mozilla {
namespace layers {

static const float EPSILON = getenv("AX_1") ? atof(getenv("AX_1")) : 0.0001f;

/**
 * Maximum acceleration that can happen between two frames. Velocity is
 * throttled if it's above this. This may happen if a time delta is very low,
 * or we get a touch point very far away from the previous position for some
 * reason.
 */
static const float MAX_EVENT_ACCELERATION = getenv("AX_2") ? atof(getenv("AX_2")) : 12.0f;

/**
 * Amount of friction applied during flings.
 */
static const float FLING_FRICTION = getenv("AX_3") ? atof(getenv("AX_3")) : 0.007f;

/**
 * Threshold for velocity beneath which we turn off any acceleration we had
 * during repeated flings.
 */
static const float VELOCITY_THRESHOLD = getenv("AX_4") ? atof(getenv("AX_4")) : 0.14f;

/**
 * Amount of acceleration we multiply in each time the user flings in one
 * direction. Every time they let go of the screen, we increase the acceleration
 * by this amount raised to the power of the amount of times they have let go,
 * times two (to make the curve steeper).  This stops if the user lets go and we
 * slow down enough, or if they put their finger down without moving it for a
 * moment (or in the opposite direction).
 */
static const float ACCELERATION_MULTIPLIER = getenv("AX_5") ? atof(getenv("AX_5")) : 1.125f;

/**
 * When flinging, if the velocity goes below this number, we just stop the
 * animation completely. This is to prevent asymptotically approaching 0
 * velocity and rerendering unnecessarily.
 */
static const float FLING_STOPPED_THRESHOLD = getenv("AX_6") ? atof(getenv("AX_6")) : 0.01f;

Axis::Axis(AsyncPanZoomController* aAsyncPanZoomController)
  : mPos(0.0f),
    mVelocity(0.0f),
    mAcceleration(0),
    mAsyncPanZoomController(aAsyncPanZoomController)
{

}

void Axis::UpdateWithTouchAtDevicePoint(int32_t aPos, const float& aTimeDelta) {
  if (mPos == aPos) {
    // Does not make sense to calculate velocity when distance is 0
    return;
  }

  float newVelocity = (mPos - aPos) / aTimeDelta;

  bool curVelocityIsLow = fabsf(newVelocity) < 0.01f;
  bool curVelocityBelowThreshold = fabsf(newVelocity) < VELOCITY_THRESHOLD;
  bool directionChange = (mVelocity > 0) != (newVelocity > 0);

  // If we've changed directions, or the current velocity threshold, stop any
  // acceleration we've accumulated.
  if (directionChange || curVelocityBelowThreshold) {
//    printf(">>>>>>Func:%s::%d newVel:%g dirCh or low, tmD:%i, mP:%i, aP:%i\n", __PRETTY_FUNCTION__, __LINE__, newVelocity, aTimeDelta, mPos, aPos);
    mAcceleration = 0;
  } else {
//    printf(">>>>>>Func:%s::%d newVel:%g\n", __PRETTY_FUNCTION__, __LINE__, newVelocity);
  }

  // If a direction change has happened, or the current velocity due to this new
  // touch is relatively low, then just apply it. If not, throttle it.
  if (curVelocityIsLow || (directionChange && fabs(newVelocity) - EPSILON <= 0.0f)) {
//    printf(">>>>>>Func:%s::%d mVel:%g->%g\n", __PRETTY_FUNCTION__, __LINE__, mVelocity, newVelocity);
    mVelocity = newVelocity;
  } else {
//    printf(">>>>>>Func:%s::%d mVel:%g->%g\n", __PRETTY_FUNCTION__, __LINE__, mVelocity, NS_MIN(NS_MAX(newVelocity, -MAX_EVENT_ACCELERATION), MAX_EVENT_ACCELERATION));
    mVelocity = NS_MIN(NS_MAX(newVelocity, -MAX_EVENT_ACCELERATION), MAX_EVENT_ACCELERATION);
  }
  

  mPos = aPos;
}

void Axis::StartTouch(int32_t aPos) {
  mStartPos = aPos;
  mPos = aPos;
}

float Axis::GetDisplacementForDuration(float aScale, const float& aDelta) {
  if (fabsf(mVelocity) < VELOCITY_THRESHOLD) {
//    printf(">>>>>>Func:%s::%d mVel:%g, mAccel:->0\n", __PRETTY_FUNCTION__, __LINE__, mVelocity);
    mAcceleration = 0;
  }

  float accelerationFactor = GetAccelerationFactor();
  float displacement = mVelocity * aScale * aDelta * accelerationFactor;
  // If this displacement will cause an overscroll, throttle it. Can potentially
  // bring it to 0 even if the velocity is high.
  if (DisplacementWillOverscroll(displacement) != OVERSCROLL_NONE) {
    // No need to have a velocity along this axis anymore; it won't take us
    // anywhere, so we're just spinning needlessly.
//    printf(">>>>>>Func:%s::%d displ:%g, mVel:%g, sc:%g, delt:%g, accelFc:%g\n", __PRETTY_FUNCTION__, __LINE__, displacement, mVelocity, aScale, aDelta, accelerationFactor);
    mVelocity = 0.0f;
    mAcceleration = 0;
    displacement -= DisplacementWillOverscrollAmount(displacement);
  } else {
//    printf(">>>>>>Func:%s::%d displ:%g, mVel:%g, sc:%g, delt:%g, accelFc:%g\n", __PRETTY_FUNCTION__, __LINE__, displacement, mVelocity, aScale, aDelta, accelerationFactor);
  }
  return displacement;
}

float Axis::PanDistance() {
  return fabsf(mPos - mStartPos);
}

void Axis::EndTouch() {
  mAcceleration++;
//  printf(">>>>>>Func:%s::%d mVel:%g, mAccel:%g\n", __PRETTY_FUNCTION__, __LINE__, mVelocity, mAcceleration);
}

void Axis::CancelTouch() {
  mVelocity = 0.0f;
  mAcceleration = 0;
}

bool Axis::FlingApplyFrictionOrCancel(const float& aDelta) {
  if (fabsf(mVelocity) <= FLING_STOPPED_THRESHOLD) {
    // If the velocity is very low, just set it to 0 and stop the fling,
    // otherwise we'll just asymptotically approach 0 and the user won't
    // actually see any changes.
    mVelocity = 0.0f;
    return false;
  } else {
    mVelocity *= NS_MAX(1.0f - FLING_FRICTION * aDelta, 0.0f);
  }
  return true;
}

Axis::Overscroll Axis::GetOverscroll() {
  // If the current pan takes the window to the left of or above the current
  // page rect.
  bool minus = GetOrigin() < GetPageStart();
  // If the current pan takes the window to the right of or below the current
  // page rect.
  bool plus = GetCompositionEnd() > GetPageEnd();
  if (minus && plus) {
    return OVERSCROLL_BOTH;
  }
  if (minus) {
    return OVERSCROLL_MINUS;
  }
  if (plus) {
    return OVERSCROLL_PLUS;
  }
  return OVERSCROLL_NONE;
}

float Axis::GetExcess() {
  switch (GetOverscroll()) {
  case OVERSCROLL_MINUS: return GetOrigin() - GetPageStart();
  case OVERSCROLL_PLUS: return GetCompositionEnd() - GetPageEnd();
  case OVERSCROLL_BOTH: return (GetCompositionEnd() - GetPageEnd()) +
                               (GetPageStart() - GetOrigin());
  default: return 0;
  }
}

Axis::Overscroll Axis::DisplacementWillOverscroll(int32_t aDisplacement) {
  // If the current pan plus a displacement takes the window to the left of or
  // above the current page rect.
  bool minus = GetOrigin() + aDisplacement < GetPageStart();
  // If the current pan plus a displacement takes the window to the right of or
  // below the current page rect.
  bool plus = GetCompositionEnd() + aDisplacement > GetPageEnd();
  if (minus && plus) {
    return OVERSCROLL_BOTH;
  }
  if (minus) {
    return OVERSCROLL_MINUS;
  }
  if (plus) {
    return OVERSCROLL_PLUS;
  }
  return OVERSCROLL_NONE;
}

float Axis::DisplacementWillOverscrollAmount(int32_t aDisplacement) {
  switch (DisplacementWillOverscroll(aDisplacement)) {
  case OVERSCROLL_MINUS: return (GetOrigin() + aDisplacement) - GetPageStart();
  case OVERSCROLL_PLUS: return (GetCompositionEnd() + aDisplacement) - GetPageEnd();
  // Don't handle overscrolled in both directions; a displacement can't cause
  // this, it must have already been zoomed out too far.
  default: return 0;
  }
}

Axis::Overscroll Axis::ScaleWillOverscroll(float aScale, int32_t aFocus) {
  float originAfterScale = (GetOrigin() + aFocus) * aScale - aFocus;

  bool both = ScaleWillOverscrollBothSides(aScale);
  bool minus = originAfterScale < GetPageStart() * aScale;
  bool plus = (originAfterScale + GetCompositionLength()) > GetPageEnd() * aScale;

  if ((minus && plus) || both) {
    return OVERSCROLL_BOTH;
  }
  if (minus) {
    return OVERSCROLL_MINUS;
  }
  if (plus) {
    return OVERSCROLL_PLUS;
  }
  return OVERSCROLL_NONE;
}

float Axis::ScaleWillOverscrollAmount(float aScale, int32_t aFocus) {
  float originAfterScale = (GetOrigin() + aFocus) * aScale - aFocus;
  switch (ScaleWillOverscroll(aScale, aFocus)) {
  case OVERSCROLL_MINUS: return originAfterScale - GetPageStart() * aScale;
  case OVERSCROLL_PLUS: return (originAfterScale + GetCompositionLength()) -
                               NS_lround(GetPageEnd() * aScale);
  // Don't handle OVERSCROLL_BOTH. Client code is expected to deal with it.
  default: return 0;
  }
}

float Axis::GetVelocity() {
  return mVelocity;
}

float Axis::GetAccelerationFactor() {
  return powf(ACCELERATION_MULTIPLIER, NS_MAX(0, (mAcceleration - 4) * 3));
}

float Axis::GetCompositionEnd() {
  return GetOrigin() + GetCompositionLength();
}

float Axis::GetPageEnd() {
  return GetPageStart() + GetPageLength();
}

float Axis::GetOrigin() {
  gfx::Point origin = mAsyncPanZoomController->GetFrameMetrics().mScrollOffset;
  return GetPointOffset(origin);
}

float Axis::GetCompositionLength() {
  const FrameMetrics& metrics = mAsyncPanZoomController->GetFrameMetrics();
  gfx::Rect cssCompositedRect =
    AsyncPanZoomController::CalculateCompositedRectInCssPixels(metrics);
  return GetRectLength(cssCompositedRect);
}

float Axis::GetPageStart() {
  gfx::Rect pageRect = mAsyncPanZoomController->GetFrameMetrics().mScrollableRect;
  return GetRectOffset(pageRect);
}

float Axis::GetPageLength() {
  gfx::Rect pageRect = mAsyncPanZoomController->GetFrameMetrics().mScrollableRect;
  return GetRectLength(pageRect);
}

bool Axis::ScaleWillOverscrollBothSides(float aScale) {
  const FrameMetrics& metrics = mAsyncPanZoomController->GetFrameMetrics();

  gfx::Rect cssContentRect = metrics.mScrollableRect;

  float currentScale = metrics.mZoom.width;
  nsIntRect compositionBounds = metrics.mCompositionBounds;
  gfx::Rect scaledCompositionBounds =
    gfx::Rect(compositionBounds.x, compositionBounds.y,
              compositionBounds.width, compositionBounds.height);
  scaledCompositionBounds.ScaleInverseRoundIn(currentScale * aScale);

  return GetRectLength(cssContentRect) < GetRectLength(scaledCompositionBounds);
}

AxisX::AxisX(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

float AxisX::GetPointOffset(const gfx::Point& aPoint)
{
  return aPoint.x;
}

float AxisX::GetRectLength(const gfx::Rect& aRect)
{
  return aRect.width;
}

float AxisX::GetRectOffset(const gfx::Rect& aRect)
{
  return aRect.x;
}

AxisY::AxisY(AsyncPanZoomController* aAsyncPanZoomController)
  : Axis(aAsyncPanZoomController)
{

}

float AxisY::GetPointOffset(const gfx::Point& aPoint)
{
  return aPoint.y;
}

float AxisY::GetRectLength(const gfx::Rect& aRect)
{
  return aRect.height;
}

float AxisY::GetRectOffset(const gfx::Rect& aRect)
{
  return aRect.y;
}

}
}
