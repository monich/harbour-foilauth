/*
 * Copyright (C) 2019-2022 Jolla Ltd.
 * Copyright (C) 2019-2022 Slava Monich <slava@monich.com>
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

#ifndef FOILAUTH_TOKEN_H
#define FOILAUTH_TOKEN_H

#include "FoilAuthTypes.h"

#include <QByteArray>
#include <QDebug>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

// Read-only token object, cheap to copy
class FoilAuthToken
{
    Q_GADGET
    Q_PROPERTY(bool valid READ isValid CONSTANT)
    Q_PROPERTY(int type READ type CONSTANT)
    Q_PROPERTY(int algorithm READ algorithm CONSTANT)
    Q_PROPERTY(QString label READ label CONSTANT)
    Q_PROPERTY(QString issuer READ issuer CONSTANT)
    Q_PROPERTY(QString secret READ secretBase32 CONSTANT)
    Q_PROPERTY(int digits READ digits CONSTANT)
    Q_PROPERTY(int counter READ counter CONSTANT)
    Q_PROPERTY(int timeshift READ timeshift CONSTANT)

    class Private;
    FoilAuthToken(Private*);

public:
    static const QString ALGORITHM_SHA1;
    static const QString ALGORITHM_SHA256;
    static const QString ALGORITHM_SHA512;

    static const int MIN_DIGITS = 1;
    static const int MAX_DIGITS = 9;

    static const QString KEY_VALID;
    static const QString KEY_TYPE;
    static const QString KEY_LABEL;
    static const QString KEY_SECRET;
    static const QString KEY_ISSUER;
    static const QString KEY_DIGITS;
    static const QString KEY_COUNTER;
    static const QString KEY_TIMESHIFT;
    static const QString KEY_ALGORITHM;

    static const QString TYPE_TOTP;
    static const QString TYPE_HOTP;

    FoilAuthToken();
    FoilAuthToken(const FoilAuthToken& aToken);
    FoilAuthToken(FoilAuthTypes::AuthType, QByteArray, QString, QString,
        int aDigits = FoilAuthTypes::DEFAULT_DIGITS,
        quint64 aCounter = FoilAuthTypes::DEFAULT_COUNTER,
        int aTimeshift = FoilAuthTypes::DEFAULT_TIMESHIFT,
        FoilAuthTypes::DigestAlgorithm aAlgorithm = FoilAuthTypes::DEFAULT_ALGORITHM);
    ~FoilAuthToken();

    Q_REQUIRED_RESULT static FoilAuthToken fromUri(const QString);
    Q_REQUIRED_RESULT FoilAuthToken withType(FoilAuthTypes::AuthType) const;
    Q_REQUIRED_RESULT FoilAuthToken withAlgorithm(FoilAuthTypes::DigestAlgorithm) const;
    Q_REQUIRED_RESULT FoilAuthToken withSecret(const QByteArray) const;
    Q_REQUIRED_RESULT FoilAuthToken withLabel(const QString) const;
    Q_REQUIRED_RESULT FoilAuthToken withIssuer(const QString) const;
    Q_REQUIRED_RESULT FoilAuthToken withCounter(quint64) const;
    Q_REQUIRED_RESULT FoilAuthToken withDigits(int) const;
    Q_REQUIRED_RESULT FoilAuthToken withTimeshift(int) const;

    FoilAuthToken& operator=(const FoilAuthToken& aToken);
    bool operator==(const FoilAuthToken& aToken) const;
    bool operator!=(const FoilAuthToken& aToken) const;
    bool equals(const FoilAuthToken&) const;

    bool isValid() const;
    FoilAuthTypes::AuthType type() const;
    FoilAuthTypes::DigestAlgorithm algorithm() const;
    QString label() const;
    QString issuer() const;
    QString secretBase32() const;
    const QByteArray secret() const;
    quint64 counter() const;
    int digits() const;
    int timeshift() const;
    uint password(quint64 aTime) const;
    Q_REQUIRED_RESULT QString passwordString(quint64) const;
    Q_REQUIRED_RESULT QString toUri() const;
    Q_REQUIRED_RESULT QVariantMap toVariantMap() const;
    Q_REQUIRED_RESULT QByteArray toProtoBuf() const;

    Q_REQUIRED_RESULT static FoilAuthTypes::AuthType validType(int);
    Q_REQUIRED_RESULT static FoilAuthTypes::DigestAlgorithm validAlgorithm(int);
    Q_REQUIRED_RESULT static int validDigits(int);
    Q_REQUIRED_RESULT static QList<FoilAuthToken> fromProtoBuf(const QByteArray&);
    Q_REQUIRED_RESULT static QByteArray toProtoBuf(const QList<FoilAuthToken>&);
    Q_REQUIRED_RESULT static QList<QByteArray> toProtoBufs(const QList<FoilAuthToken>&,
        int aPrefBatchSize = 1000, int aMaxBatchSize = 2000);

public:
    Private* iPrivate;
};

inline bool FoilAuthToken::operator==(const FoilAuthToken& aToken) const
    { return equals(aToken); }
inline bool FoilAuthToken::operator!=(const FoilAuthToken& aToken) const
    { return !equals(aToken); }

// Debug output
QDebug operator<<(QDebug, const FoilAuthToken&);

Q_DECLARE_METATYPE(FoilAuthToken)

#endif // FOILAUTH_TOKEN_H
