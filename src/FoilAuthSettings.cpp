/*
 * Copyright (C) 2019-2020 Jolla Ltd.
 * Copyright (C) 2019-2020 Slava Monich <slava@monich.com>
 *
 * You may use this file under the terms of BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FoilAuthSettings.h"
#include "FoilAuthDefs.h"

#include "HarbourDebug.h"

#include <MGConfItem>

#define DCONF_KEY(x)                FOILAUTH_DCONF_ROOT x
#define KEY_MAX_ZOOM                DCONF_KEY("maxZoom")
#define KEY_SCAN_ZOOM               DCONF_KEY("scanZoom")
#define KEY_SCAN_WIDE_MODE          DCONF_KEY("scanWideMode")
#define KEY_SAILOTP_IMPORT_DONE     DCONF_KEY("sailotpImportDone")
#define KEY_SHARED_KEY_WARNING      DCONF_KEY("sharedKeyWarning")
#define KEY_SHARED_KEY_WARNING2     DCONF_KEY("sharedKeyWarning2")
#define KEY_AUTO_LOCK_TIME          DCONF_KEY("autoLockTime")

#define DEFAULT_MAX_ZOOM            10.f
#define DEFAULT_SCAN_ZOOM           3.f
#define DEFAULT_SCAN_WIDE_MODE      false
#define DEFAULT_SAILOTP_IMPORT_DONE false
#define DEFAULT_SHARED_KEY_WARNING  true
#define DEFAULT_AUTO_LOCK_TIME      15000

// ==========================================================================
// FoilAuthSettings::Private
// ==========================================================================

class FoilAuthSettings::Private {
public:
    Private(FoilAuthSettings* aParent);

public:
    MGConfItem* iMaxZoom;
    MGConfItem* iScanZoom;
    MGConfItem* iScanWideMode;
    MGConfItem* iSailotpImportDone;
    MGConfItem* iSharedKeyWarning;
    MGConfItem* iSharedKeyWarning2;
    MGConfItem* iAutoLockTime;
    QVariant iDefaultSharedKeyWarning;
    QVariant iDefaultAutoLockTime;
};

FoilAuthSettings::Private::Private(FoilAuthSettings* aParent) :
    iMaxZoom(new MGConfItem(KEY_MAX_ZOOM, aParent)),
    iScanZoom(new MGConfItem(KEY_SCAN_ZOOM, aParent)),
    iScanWideMode(new MGConfItem(KEY_SCAN_WIDE_MODE, aParent)),
    iSailotpImportDone(new MGConfItem(KEY_SAILOTP_IMPORT_DONE, aParent)),
    iSharedKeyWarning(new MGConfItem(KEY_SHARED_KEY_WARNING, aParent)),
    iSharedKeyWarning2(new MGConfItem(KEY_SHARED_KEY_WARNING2, aParent)),
    iAutoLockTime(new MGConfItem(KEY_AUTO_LOCK_TIME, aParent)),
    iDefaultSharedKeyWarning(DEFAULT_SHARED_KEY_WARNING),
    iDefaultAutoLockTime(DEFAULT_AUTO_LOCK_TIME)
{
    QObject::connect(iMaxZoom, SIGNAL(valueChanged()),
        aParent, SIGNAL(maxZoomChanged()));
    QObject::connect(iScanZoom, SIGNAL(valueChanged()),
        aParent, SIGNAL(scanZoomChanged()));
    QObject::connect(iScanWideMode, SIGNAL(valueChanged()),
        aParent, SIGNAL(scanWideModeChanged()));
    QObject::connect(iSailotpImportDone, SIGNAL(valueChanged()),
        aParent, SIGNAL(sailotpImportDoneChanged()));
    QObject::connect(iSharedKeyWarning, SIGNAL(valueChanged()),
        aParent, SIGNAL(sharedKeyWarningChanged()));
    QObject::connect(iSharedKeyWarning2, SIGNAL(valueChanged()),
        aParent, SIGNAL(sharedKeyWarning2Changed()));
    QObject::connect(iAutoLockTime, SIGNAL(valueChanged()),
        aParent, SIGNAL(autoLockTimeChanged()));
}

// ==========================================================================
// FoilAuthSettings
// ==========================================================================

FoilAuthSettings::FoilAuthSettings(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

FoilAuthSettings::~FoilAuthSettings()
{
    delete iPrivate;
}

// Callback for qmlRegisterSingletonType<FoilAuthSettings>
QObject*
FoilAuthSettings::createSingleton(
    QQmlEngine* aEngine,
    QJSEngine* aScript)
{
    return new FoilAuthSettings;
}

// scanZoom

qreal
FoilAuthSettings::scanZoom() const
{
    return iPrivate->iScanZoom->value(DEFAULT_SCAN_ZOOM).toFloat();
}

void
FoilAuthSettings::setScanZoom(
    qreal aValue)
{
    HDEBUG(aValue);
    iPrivate->iScanZoom->set(aValue);
}

// maxZoom

qreal
FoilAuthSettings::maxZoom() const
{
    return iPrivate->iMaxZoom->value(DEFAULT_MAX_ZOOM).toFloat();
}

void
FoilAuthSettings::setMaxZoom(
    qreal aValue)
{
    HDEBUG(aValue);
    iPrivate->iMaxZoom->set(aValue);
}

// scanWideMode

bool
FoilAuthSettings::scanWideMode() const
{
    return iPrivate->iScanWideMode->value(DEFAULT_SCAN_WIDE_MODE).toBool();
}

void
FoilAuthSettings::setScanWideMode(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iScanWideMode->set(aValue);
}

// sailotpImportDone

bool
FoilAuthSettings::sailotpImportDone() const
{
    return iPrivate->iSailotpImportDone->value(DEFAULT_SAILOTP_IMPORT_DONE).toBool();
}

void
FoilAuthSettings::setSailotpImportDone(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iSailotpImportDone->set(aValue);
}

// sharedKeyWarning
// sharedKeyWarning2

bool
FoilAuthSettings::sharedKeyWarning() const
{
    return iPrivate->iSharedKeyWarning->value(iPrivate->iDefaultSharedKeyWarning).toBool();
}

bool
FoilAuthSettings::sharedKeyWarning2() const
{
    return iPrivate->iSharedKeyWarning2->value(iPrivate->iDefaultSharedKeyWarning).toBool();
}

void
FoilAuthSettings::setSharedKeyWarning(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iSharedKeyWarning->set(aValue);
}

void
FoilAuthSettings::setSharedKeyWarning2(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iSharedKeyWarning2->set(aValue);
}

// autoLockTime

int
FoilAuthSettings::autoLockTime() const
{
    QVariant val(iPrivate->iAutoLockTime->value(iPrivate->iDefaultAutoLockTime));
    bool ok;
    const int ival(val.toInt(&ok));
    return (ok && ival >= 0) ? ival : DEFAULT_AUTO_LOCK_TIME;
}

void
FoilAuthSettings::setAutoLockTime(
    int aValue)
{
    HDEBUG(aValue);
    iPrivate->iAutoLockTime->set(aValue);
}
