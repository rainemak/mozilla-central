/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qmozcontext_h
#define qmozcontext_h

#include <QObject>

class QMozContextPrivate;

class QMozContext : public QObject
{
    Q_OBJECT
public:
    QMozContext(QObject* parent = 0);
    virtual ~QMozContext();

private:
    QMozContextPrivate* d;
};

#endif /* qmozcontext_h */
