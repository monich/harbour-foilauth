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

#ifndef FOILAUTH_SETTINGS_H
#define FOILAUTH_SETTINGS_H

#include <QtQml>

class FoilAuthSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal scanZoom READ scanZoom WRITE setScanZoom NOTIFY scanZoomChanged)
    Q_PROPERTY(qreal maxZoom READ maxZoom WRITE setMaxZoom NOTIFY maxZoomChanged)
    Q_PROPERTY(bool scanWideMode READ scanWideMode WRITE setScanWideMode NOTIFY scanWideModeChanged)
    Q_PROPERTY(bool sailotpImportDone READ sailotpImportDone WRITE setSailotpImportDone NOTIFY sailotpImportDoneChanged)
    Q_PROPERTY(bool sharedKeyWarning READ sharedKeyWarning WRITE setSharedKeyWarning NOTIFY sharedKeyWarningChanged)
    Q_PROPERTY(bool sharedKeyWarning2 READ sharedKeyWarning2 WRITE setSharedKeyWarning2 NOTIFY sharedKeyWarning2Changed)
    Q_PROPERTY(int autoLockTime READ autoLockTime WRITE setAutoLockTime NOTIFY autoLockTimeChanged)
    Q_DISABLE_COPY(FoilAuthSettings)

public:
    explicit FoilAuthSettings(QObject* aParent = Q_NULLPTR);
    ~FoilAuthSettings();

    static QObject* createSingleton(QQmlEngine* aEngine, QJSEngine* aScript);

    qreal scanZoom() const;
    void setScanZoom(qreal aValue);

    qreal maxZoom() const;
    void setMaxZoom(qreal aValue);

    bool scanWideMode() const;
    void setScanWideMode(bool aValue);

    bool sailotpImportDone() const;
    void setSailotpImportDone(bool aValue);

    bool sharedKeyWarning() const;
    bool sharedKeyWarning2() const;
    void setSharedKeyWarning(bool aValue);
    void setSharedKeyWarning2(bool aValue);

    int autoLockTime() const;
    void setAutoLockTime(int aValue);

Q_SIGNALS:
    void maxZoomChanged();
    void scanZoomChanged();
    void scanWideModeChanged();
    void sailotpImportDoneChanged();
    void sharedKeyWarningChanged();
    void sharedKeyWarning2Changed();
    void autoLockTimeChanged();

private:
    class Private;
    Private* iPrivate;
};

#endif // FOILAUTH_SETTINGS_H
