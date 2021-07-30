/*
 * Copyright (C) 2019-2021 Jolla Ltd.
 * Copyright (C) 2019-2021 Slava Monich <slava@monich.com>
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

#ifndef FOILAUTH_H
#define FOILAUTH_H

#include "FoilAuthTypes.h"

#include "foil_types.h"

#include <QSize>
#include <QObject>
#include <QByteArray>
#include <QVariantMap>
#include <QVariantList>

class QQmlEngine;
class QJSEngine;

// Static utilities
class FoilAuth : public QObject, public FoilAuthTypes {
    Q_OBJECT
    Q_PROPERTY(bool otherFoilAppsInstalled READ otherFoilAppsInstalled NOTIFY otherFoilAppsInstalledChanged)
    Q_DISABLE_COPY(FoilAuth)
    Q_ENUMS(Algorithm)

public:
    static const int PERIOD = 30;

    // Export these to QML
    enum Algorithm {
        AlgorithmMD5 = DigestAlgorithmMD5,
        AlgorithmSHA1 = DigestAlgorithmSHA1,
        AlgorithmSHA256 = DigestAlgorithmSHA256,
        AlgorithmSHA512 = DigestAlgorithmSHA512,
        DefaultAlgorithm = DEFAULT_ALGORITHM
    };

    explicit FoilAuth(QObject* aParent = Q_NULLPTR);

    // Callback for qmlRegisterSingletonType<FoilAuth>
    static QObject* createSingleton(QQmlEngine* aEngine, QJSEngine* aScript);

    bool otherFoilAppsInstalled() const;

    // Static utilities
    static QSize toSize(QVariant aVariant);
    static QByteArray fromBase32(QString aBase32);
    static QString toBase32(QByteArray aBinary, bool aLowerCase = true);
    static QByteArray toByteArray(GBytes* aData);
    static FoilOutput* createFoilFile(QString aDestDir, GString* aOutPath);
    static QString createEmptyFoilFile(QString aDestDir);
    static QString migrationUri(QByteArray aData);
    static uint TOTP(QByteArray aSecret, quint64 aTime, uint aMaxPass,
        DigestAlgorithm aAlgorithm = DEFAULT_ALGORITHM);

    // Invokable from QML
    Q_INVOKABLE static QString toUri(QString aSecretBase32, QString aLabel,
        QString aIssuer, int aDigits, int aTimeShift, Algorithm aAlgorithm);
    Q_INVOKABLE static QVariantMap parseUri(QString aUri);
    Q_INVOKABLE static QVariantList parseMigrationUri(QString aUri);
    Q_INVOKABLE static bool isValidBase32(QString aBase32);
    Q_INVOKABLE static QStringList stringListRemove(QStringList aList, QString aString);

Q_SIGNALS:
    void otherFoilAppsInstalledChanged();

private:
    class Private;
    Private* iPrivate;
};

#endif // FOILAUTH_H
