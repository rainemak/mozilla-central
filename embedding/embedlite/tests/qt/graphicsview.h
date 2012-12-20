/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=4 sw=4 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QGraphicsView>
#include <QtOpenGL/QGLWidget>
#include <sys/time.h>
#include <stdio.h>

class QGraphicsWidget;

class MozFPSCounter
{
public:
    MozFPSCounter(const char* aEnv = NULL, const char* aMsg = NULL, int aFreq = 5)
        : mEnv(aEnv), mMsg(aMsg), mFreq(aFreq), mFpsCounter(0), mEnabled(true) {
        if (mEnv) {
            char* envVal = getenv(mEnv);
            if (!envVal) {
                mEnabled = false;
            }
            if (envVal && *envVal) {
                mFreq = atoi(envVal);
            }
        }
    }
    void Count(void) {
        if (!mEnabled) return;
        if (!mFpsCounter) {
            gettimeofday(&mPrevTime, NULL);
        }
        mFpsCounter++;
        if (mFpsCounter > mFreq) {
            gettimeofday(&mCurrTime, NULL);
            timersub(&mCurrTime, &mPrevTime, &mDiffTime);
            float time = mFpsCounter / (mDiffTime.tv_sec + (float)mDiffTime.tv_usec/1000000);
            printf("%s: time:%ld.%06ld fps:%g\n", mMsg, mDiffTime.tv_sec, mDiffTime.tv_usec, time);
            mFpsCounter = 0;
        }
    }
    virtual ~MozFPSCounter() {};
private:
    const char* mEnv;
    const char* mMsg;
    int mFreq;
    int mFpsCounter;
    bool mEnabled;
    struct timeval mDiffTime;
    struct timeval mCurrTime;
    struct timeval mPrevTime;
};

class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    MyGraphicsView(QGraphicsScene*, QGraphicsWidget*);
    virtual void resizeEvent(QResizeEvent*);
    virtual void paintEvent(QPaintEvent*);
    void SetGLWidget();

private:
    QGraphicsWidget* mTopLevel;
    QGLWidget* mGL;
};
