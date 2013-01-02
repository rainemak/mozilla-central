/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozcontext.h"

class QMozContextPrivate {
public:
    QMozContextPrivate(QMozContext*);
    ~QMozContextPrivate();

    QMozContext* q;
};

QMozContextPrivate::QMozContextPrivate(QMozContext* qq)
    : q(qq)
{
}

QMozContextPrivate::~QMozContextPrivate()
{
}

QMozContext::QMozContext(QObject* parent)
    : QObject(parent)
    , d(new QMozContextPrivate(this))
{
}

QMozContext::~QMozContext()
{
    delete d;
}
