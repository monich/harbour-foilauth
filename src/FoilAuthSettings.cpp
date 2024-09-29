/*
 * Copyright (C) 2019-2024 Slava Monich <slava@monich.com>
 * Copyright (C) 2019-2022 Jolla Ltd.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer
 *     in the documentation and/or other materials provided with the
 *     distribution.
 *
 *  3. Neither the names of the copyright holders nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#include "FoilAuthSettings.h"
#include "FoilAuthDefs.h"

#include "HarbourQrCodeGenerator.h"
#include "HarbourDebug.h"

#include <MGConfItem>

#define DCONF_KEY(x)                FOILAUTH_DCONF_ROOT x
#define KEY_RESOLUTION_4_3          DCONF_KEY("resolution_4_3")  // Width is stored
#define KEY_RESOLUTION_16_9         DCONF_KEY("resolution_16_9") // Width is stored
#define KEY_QRCODE_ECLEVEL          DCONF_KEY("qrCodeEcLevel")
#define KEY_MAX_ZOOM                DCONF_KEY("maxZoom")
#define KEY_SCAN_ZOOM               DCONF_KEY("scanZoom")
#define KEY_VOLUME_ZOOM             DCONF_KEY("volumeZoom")
#define KEY_FRONT_CAMERA            DCONF_KEY("frontCamera")
#define KEY_SCAN_WIDE_MODE          DCONF_KEY("scanWideMode")
#define KEY_SHARED_KEY_WARNING      DCONF_KEY("sharedKeyWarning")
#define KEY_SHARED_KEY_WARNING2     DCONF_KEY("sharedKeyWarning2")
#define KEY_AUTO_LOCK               DCONF_KEY("autoLock")
#define KEY_AUTO_LOCK_TIME          DCONF_KEY("autoLockTime")
#define KEY_SAILOTP_IMPORT_DONE     DCONF_KEY("sailotpImportDone")
#define KEY_SAILOTP_IMPORTED_TOKENS DCONF_KEY("sailotpImportedTokens")

#define DEFAULT_QRCODE_ECLEVEL      ((int)HarbourQrCodeGenerator::ECLevelLowest)
#define DEFAULT_MAX_ZOOM            10.f
#define DEFAULT_SCAN_ZOOM           3.f
#define DEFAULT_VOLUME_ZOOM         true
#define DEFAULT_FRONT_CAMERA        false
#define DEFAULT_SCAN_WIDE_MODE      false
#define DEFAULT_SHARED_KEY_WARNING  true
#define DEFAULT_AUTO_LOCK           true
#define DEFAULT_AUTO_LOCK_TIME      15000

// Camera configuration (got removed at some point)
#define CAMERA_DCONF_PATH_(x)           "/apps/jolla-camera/primary/image/" x
#define CAMERA_DCONF_RESOLUTION_4_3     CAMERA_DCONF_PATH_("viewfinderResolution_4_3")
#define CAMERA_DCONF_RESOLUTION_16_9    CAMERA_DCONF_PATH_("viewfinderResolution_16_9")

// ==========================================================================
// FoilAuthSettings::Private
// ==========================================================================

class FoilAuthSettings::Private
{
public:
    Private(FoilAuthSettings*);

    static int validateQrCodeEcLevel(int);
    static QSize toSize(const QVariant);
    static QSize size_4_3(int);
    static QSize size_16_9(int);

    QSize resolution_4_3();
    QSize resolution_16_9();

public:
    const int iDefaultResolution_4_3;
    const int iDefaultResolution_16_9;
    MGConfItem* iResolution_4_3;
    MGConfItem* iResolution_16_9;
    MGConfItem* iQrCodeEcLevel;
    MGConfItem* iMaxZoom;
    MGConfItem* iScanZoom;
    MGConfItem* iVolumeZoom;
    MGConfItem* iFrontCamera;
    MGConfItem* iScanWideMode;
    MGConfItem* iSharedKeyWarning;
    MGConfItem* iSharedKeyWarning2;
    MGConfItem* iAutoLock;
    MGConfItem* iAutoLockTime;
    MGConfItem* iSailotpImportDone;
    MGConfItem* iSailotpImportedTokens;
};

FoilAuthSettings::Private::Private(
    FoilAuthSettings* aParent) :
    iDefaultResolution_4_3(toSize(MGConfItem(CAMERA_DCONF_RESOLUTION_4_3).value()).width()),
    iDefaultResolution_16_9(toSize(MGConfItem(CAMERA_DCONF_RESOLUTION_16_9).value()).width()),
    iResolution_4_3(new MGConfItem(KEY_RESOLUTION_4_3, aParent)),
    iResolution_16_9(new MGConfItem(KEY_RESOLUTION_16_9, aParent)),
    iQrCodeEcLevel(new MGConfItem(KEY_QRCODE_ECLEVEL, aParent)),
    iMaxZoom(new MGConfItem(KEY_MAX_ZOOM, aParent)),
    iScanZoom(new MGConfItem(KEY_SCAN_ZOOM, aParent)),
    iVolumeZoom(new MGConfItem(KEY_VOLUME_ZOOM, aParent)),
    iFrontCamera(new MGConfItem(KEY_FRONT_CAMERA, aParent)),
    iScanWideMode(new MGConfItem(KEY_SCAN_WIDE_MODE, aParent)),
    iSharedKeyWarning(new MGConfItem(KEY_SHARED_KEY_WARNING, aParent)),
    iSharedKeyWarning2(new MGConfItem(KEY_SHARED_KEY_WARNING2, aParent)),
    iAutoLock(new MGConfItem(KEY_AUTO_LOCK, aParent)),
    iAutoLockTime(new MGConfItem(KEY_AUTO_LOCK_TIME, aParent)),
    iSailotpImportDone(new MGConfItem(KEY_SAILOTP_IMPORT_DONE, aParent)),
    iSailotpImportedTokens(new MGConfItem(KEY_SAILOTP_IMPORTED_TOKENS, aParent))
{
    connect(iResolution_4_3, SIGNAL(valueChanged()), aParent, SIGNAL(wideCameraResolutionChanged()));
    connect(iResolution_16_9, SIGNAL(valueChanged()), aParent, SIGNAL(narrowCameraResolutionChanged()));
    connect(iQrCodeEcLevel, SIGNAL(valueChanged()), aParent, SIGNAL(qrCodeEcLevelChanged()));
    connect(iMaxZoom, SIGNAL(valueChanged()), aParent, SIGNAL(maxZoomChanged()));
    connect(iScanZoom, SIGNAL(valueChanged()), aParent, SIGNAL(scanZoomChanged()));
    connect(iVolumeZoom, SIGNAL(valueChanged()), aParent, SIGNAL(volumeZoomChanged()));
    connect(iFrontCamera, SIGNAL(valueChanged()), aParent, SIGNAL(frontCameraChanged()));
    connect(iScanWideMode, SIGNAL(valueChanged()), aParent, SIGNAL(scanWideModeChanged()));
    connect(iSharedKeyWarning, SIGNAL(valueChanged()), aParent, SIGNAL(sharedKeyWarningChanged()));
    connect(iSharedKeyWarning2, SIGNAL(valueChanged()), aParent, SIGNAL(sharedKeyWarning2Changed()));
    connect(iAutoLock, SIGNAL(valueChanged()), aParent, SIGNAL(autoLockChanged()));
    connect(iAutoLockTime, SIGNAL(valueChanged()), aParent, SIGNAL(autoLockTimeChanged()));
    connect(iSailotpImportDone, SIGNAL(valueChanged()), aParent, SIGNAL(sailotpImportDoneChanged()));
    connect(iSailotpImportedTokens, SIGNAL(valueChanged()), aParent, SIGNAL(sailotpImportedTokensChanged()));
    HDEBUG("Default 4:3 resolution" << size_4_3(iDefaultResolution_4_3));
    HDEBUG("Default 16:9 resolution" << size_16_9(iDefaultResolution_16_9));
}

inline int
FoilAuthSettings::Private::validateQrCodeEcLevel(
    int aValue)
{
    return (aValue >= (int)HarbourQrCodeGenerator::ECLevelLowest &&
            aValue <= (int)HarbourQrCodeGenerator::ECLevelHighest) ?
            aValue : DEFAULT_QRCODE_ECLEVEL;
}

QSize
FoilAuthSettings::Private::toSize(
    const QVariant aVariant)
{
    // e.g. "1920x1080"
    if (aVariant.isValid()) {
        const QStringList values(aVariant.toString().split('x'));
        if (values.count() == 2) {
            bool ok = false;
            int width = values.at(0).toInt(&ok);
            if (ok && width > 0) {
                int height = values.at(1).toInt(&ok);
                if (ok && height > 0) {
                    return QSize(width, height);
                }
            }
        }
    }
    return QSize(0, 0);
}

QSize
FoilAuthSettings::Private::size_4_3(
    int aWidth)
{
    return QSize(aWidth, aWidth * 3 / 4);
}

QSize
FoilAuthSettings::Private::size_16_9(
    int aWidth)
{
    return QSize(aWidth, aWidth * 9 / 16);
}

QSize
FoilAuthSettings::Private::resolution_4_3()
{
    return size_4_3(qMax(iResolution_4_3->value(iDefaultResolution_4_3).toInt(), 0));
}

QSize
FoilAuthSettings::Private::resolution_16_9()
{
    return size_16_9(qMax(iResolution_16_9->value(iDefaultResolution_16_9).toInt(), 0));
}

// ==========================================================================
// FoilAuthSettings
// ==========================================================================

FoilAuthSettings::FoilAuthSettings(
    QObject* aParent) :
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
    QQmlEngine*,
    QJSEngine*)
{
    return new FoilAuthSettings;
}

// qrCodeEcLevel

int
FoilAuthSettings::qrCodeEcLevel() const
{
    return Private::validateQrCodeEcLevel(iPrivate->iQrCodeEcLevel->
        value(DEFAULT_QRCODE_ECLEVEL).toInt());
}

void
FoilAuthSettings::setQrCodeEcLevel(
    int aValue)
{
    HDEBUG(aValue);
    iPrivate->iQrCodeEcLevel->set(Private::validateQrCodeEcLevel(aValue));
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

// volumeZoom

bool
FoilAuthSettings::volumeZoom() const
{
    return iPrivate->iVolumeZoom->value(DEFAULT_VOLUME_ZOOM).toBool();
}

void
FoilAuthSettings::setVolumeZoom(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iVolumeZoom->set(aValue);
}

// frontCamera

bool
FoilAuthSettings::frontCamera() const
{
    return iPrivate->iFrontCamera->value(DEFAULT_FRONT_CAMERA).toBool();
}

void
FoilAuthSettings::setFrontCamera(
    bool aValue)
{
    iPrivate->iFrontCamera->set(aValue);
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

// wideCameraRatio

qreal
FoilAuthSettings::wideCameraRatio() const
{
    return 4./3;
}

// wideCameraResolution

QSize
FoilAuthSettings::wideCameraResolution() const
{
    return iPrivate->resolution_4_3();
}

void
FoilAuthSettings::setWideCameraResolution(
    QSize aSize)
{
    HDEBUG(aSize);
    iPrivate->iResolution_4_3->set(aSize.width());
}

// narrowCameraRatio

qreal
FoilAuthSettings::narrowCameraRatio() const
{
    return 16./9;
}

QSize
FoilAuthSettings::narrowCameraResolution() const
{
    return iPrivate->resolution_16_9();
}

void
FoilAuthSettings::setNarrowCameraResolution(
    QSize aSize)
{
    HDEBUG(aSize);
    iPrivate->iResolution_16_9->set(aSize.width());
}

// sharedKeyWarning
// sharedKeyWarning2

bool
FoilAuthSettings::sharedKeyWarning() const
{
    return iPrivate->iSharedKeyWarning->value(DEFAULT_SHARED_KEY_WARNING).toBool();
}

bool
FoilAuthSettings::sharedKeyWarning2() const
{
    return iPrivate->iSharedKeyWarning2->value(DEFAULT_SHARED_KEY_WARNING).toBool();
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

// autoLock

bool
FoilAuthSettings::autoLock() const
{
    return iPrivate->iAutoLock->value(DEFAULT_AUTO_LOCK).toBool();
}

void
FoilAuthSettings::setAutoLock(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iAutoLock->set(aValue);
}

// autoLockTime

int
FoilAuthSettings::autoLockTime() const
{
    QVariant val(iPrivate->iAutoLockTime->value(DEFAULT_AUTO_LOCK_TIME));
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

// sailotpImportDone

bool
FoilAuthSettings::sailotpImportDone() const
{
    return iPrivate->iSailotpImportDone->value(false).toBool();
}

void
FoilAuthSettings::setSailotpImportDone(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iSailotpImportDone->set(aValue);
}

// sailotpImportedTokens

QStringList
FoilAuthSettings::sailotpImportedTokens() const
{
    return iPrivate->iSailotpImportedTokens->value(QStringList()).toStringList();
}

void
FoilAuthSettings::setSailotpImportedTokens(
    QStringList aValue)
{
    HDEBUG(aValue);
    iPrivate->iSailotpImportedTokens->set(aValue);
}
