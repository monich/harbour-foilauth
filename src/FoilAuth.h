/*
 * Copyright (C) 2019-2025 Slava Monich <slava@monich.com>
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

#ifndef FOILAUTH_H
#define FOILAUTH_H

#include "FoilAuthToken.h"

#include "foil_types.h"

#include <QSize>
#include <QObject>
#include <QByteArray>
#include <QVariantMap>
#include <QVariantList>

class QQmlEngine;
class QJSEngine;

// Static utilities
class FoilAuth :
    public QObject,
    public FoilAuthTypes
{
    Q_OBJECT
    Q_PROPERTY(bool otherFoilAppsInstalled READ otherFoilAppsInstalled NOTIFY otherFoilAppsInstalledChanged)
    Q_DISABLE_COPY(FoilAuth)
    Q_ENUMS(Algorithm)
    Q_ENUMS(Type)

public:
    static const int PERIOD = 30;

    // Export these to QML
    enum Algorithm {
        AlgorithmSHA1 = DigestAlgorithmSHA1,
        AlgorithmSHA256 = DigestAlgorithmSHA256,
        AlgorithmSHA512 = DigestAlgorithmSHA512,
        DefaultAlgorithm = DEFAULT_ALGORITHM
    };

    enum Type {
        TypeTOTP = AuthTypeTOTP,
        TypeHOTP = AuthTypeHOTP,
        TypeSteam = AuthTypeSteam,
        DefaultType = DEFAULT_AUTH_TYPE
    };

    explicit FoilAuth(QObject* aParent = Q_NULLPTR);

    // Callback for qmlRegisterSingletonType<FoilAuth>
    static QObject* createSingleton(QQmlEngine*, QJSEngine*);

    bool otherFoilAppsInstalled() const;

    // Static utilities
    static QSize toSize(QVariant);
    static QString toBase32(QByteArray);
    static QByteArray fromBase32(const QString);
    static QByteArray toByteArray(GBytes*);
    static const char* generateId(GString*);
    static FoilOutput* createFoilFile(const QString, GString*);
    static QString createEmptyFoilFile(const QString);
    static QString migrationUri(const QByteArray);
    static uint TOTP(const QByteArray, quint64 aTime, uint aMaxPass,
        DigestAlgorithm aAlgorithm = DEFAULT_ALGORITHM);
    static uint HOTP(const QByteArray, quint64 aCounter, uint aMaxPass,
        DigestAlgorithm aAlgorithm = DEFAULT_ALGORITHM);
    static uint hash(const QByteArray, quint64 aValue,
        DigestAlgorithm aAlgorithm = DEFAULT_ALGORITHM);

    // Invokable from QML
    Q_INVOKABLE static QString toUri(Type aType, const QString aSecretBase32,
        const QString aLabel, const QString aIssuer, int aDigits,
        quint64 aCounter, int aTimeShift, Algorithm);
    Q_INVOKABLE static FoilAuthToken parseUri(const QString);
    Q_INVOKABLE static QList<FoilAuthToken> parseMigrationUri(const QString);
    Q_INVOKABLE static bool isValidBase32(const QString);
    Q_INVOKABLE static QStringList stringListRemove(QStringList, const QString);

Q_SIGNALS:
    void otherFoilAppsInstalledChanged();

private:
    class Private;
    Private* iPrivate;
};

#endif // FOILAUTH_H
