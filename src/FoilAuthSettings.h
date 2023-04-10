/*
 * Copyright (C) 2019-2023 Slava Monich <slava@monich.com>
 * Copyright (C) 2019-2022 Jolla Ltd.
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

#ifndef FOILAUTH_SETTINGS_H
#define FOILAUTH_SETTINGS_H

#include <QObject>
#include <QSize>
#include <QStringList>

class QQmlEngine;
class QJSEngine;

// Note that sailotpImportedTokens stores SHA1 hash of the secrets
// imported from SailOTP, not the actual secrets. In other words,
// it's not much of a security flaw.

class FoilAuthSettings :
    public QObject
{
    Q_OBJECT
    Q_PROPERTY(int qrCodeEcLevel READ qrCodeEcLevel WRITE setQrCodeEcLevel NOTIFY qrCodeEcLevelChanged)
    Q_PROPERTY(qreal maxZoom READ maxZoom WRITE setMaxZoom NOTIFY maxZoomChanged)
    Q_PROPERTY(qreal scanZoom READ scanZoom WRITE setScanZoom NOTIFY scanZoomChanged)
    Q_PROPERTY(bool volumeZoom READ volumeZoom WRITE setVolumeZoom NOTIFY volumeZoomChanged)
    Q_PROPERTY(bool scanWideMode READ scanWideMode WRITE setScanWideMode NOTIFY scanWideModeChanged)
    Q_PROPERTY(qreal wideCameraRatio READ wideCameraRatio CONSTANT)
    Q_PROPERTY(qreal narrowCameraRatio READ narrowCameraRatio CONSTANT)
    Q_PROPERTY(QSize wideCameraResolution READ wideCameraResolution WRITE setWideCameraResolution NOTIFY wideCameraResolutionChanged)
    Q_PROPERTY(QSize narrowCameraResolution READ narrowCameraResolution WRITE setNarrowCameraResolution NOTIFY narrowCameraResolutionChanged)
    Q_PROPERTY(bool sharedKeyWarning READ sharedKeyWarning WRITE setSharedKeyWarning NOTIFY sharedKeyWarningChanged)
    Q_PROPERTY(bool sharedKeyWarning2 READ sharedKeyWarning2 WRITE setSharedKeyWarning2 NOTIFY sharedKeyWarning2Changed)
    Q_PROPERTY(bool autoLock READ autoLock WRITE setAutoLock NOTIFY autoLockChanged)
    Q_PROPERTY(int autoLockTime READ autoLockTime WRITE setAutoLockTime NOTIFY autoLockTimeChanged)
    Q_PROPERTY(bool sailotpImportDone READ sailotpImportDone WRITE setSailotpImportDone NOTIFY sailotpImportDoneChanged)
    Q_PROPERTY(QStringList sailotpImportedTokens READ sailotpImportedTokens WRITE setSailotpImportedTokens NOTIFY sailotpImportedTokensChanged)
    Q_DISABLE_COPY(FoilAuthSettings)

public:
    explicit FoilAuthSettings(QObject* aParent = Q_NULLPTR);
    ~FoilAuthSettings();

    // Callback for qmlRegisterSingletonType<FoilAuthSettings>
    static QObject* createSingleton(QQmlEngine*, QJSEngine*);

    int qrCodeEcLevel() const;
    void setQrCodeEcLevel(int);

    qreal maxZoom() const;
    void setMaxZoom(qreal);

    qreal scanZoom() const;
    void setScanZoom(qreal);

    bool volumeZoom() const;
    void setVolumeZoom(bool);

    bool scanWideMode() const;
    void setScanWideMode(bool);

    qreal wideCameraRatio() const;
    QSize wideCameraResolution() const;
    void setWideCameraResolution(QSize);

    qreal narrowCameraRatio() const;
    QSize narrowCameraResolution() const;
    void setNarrowCameraResolution(QSize);

    bool sharedKeyWarning() const;
    bool sharedKeyWarning2() const;
    void setSharedKeyWarning(bool);
    void setSharedKeyWarning2(bool);

    bool autoLock() const;
    void setAutoLock(bool);

    int autoLockTime() const;
    void setAutoLockTime(int);

    bool sailotpImportDone() const;
    void setSailotpImportDone(bool);

    QStringList sailotpImportedTokens() const;
    void setSailotpImportedTokens(QStringList);

Q_SIGNALS:
    void qrCodeEcLevelChanged();
    void maxZoomChanged();
    void scanZoomChanged();
    void volumeZoomChanged();
    void scanWideModeChanged();
    void wideCameraResolutionChanged();
    void narrowCameraResolutionChanged();
    void sharedKeyWarningChanged();
    void sharedKeyWarning2Changed();
    void autoLockChanged();
    void autoLockTimeChanged();
    void sailotpImportDoneChanged();
    void sailotpImportedTokensChanged();

private:
    class Private;
    Private* iPrivate;
};

#endif // FOILAUTH_SETTINGS_H
