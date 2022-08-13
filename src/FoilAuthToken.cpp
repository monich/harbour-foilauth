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

#include "FoilAuthToken.h"
#include "FoilAuth.h"

#include "HarbourBase32.h"
#include "HarbourDebug.h"
#include "HarbourProtoBuf.h"

#include "qrencode.h"

#include <foil_random.h>

#include <gutil_misc.h>

#include <QAtomicInt>
#include <QUrl>

#define FOILAUTH_KEY_TYPE "type"
#define FOILAUTH_KEY_LABEL "label"
#define FOILAUTH_KEY_SECRET "secret"
#define FOILAUTH_KEY_ISSUER "issuer"
#define FOILAUTH_KEY_DIGITS "digits"
#define FOILAUTH_KEY_COUNTER "counter"
#define FOILAUTH_KEY_TIMESHIFT "timeshift"
#define FOILAUTH_KEY_ALGORITHM "algorithm"

const QString FoilAuthToken::KEY_VALID("valid");
const QString FoilAuthToken::KEY_TYPE(FOILAUTH_KEY_TYPE);
const QString FoilAuthToken::KEY_LABEL(FOILAUTH_KEY_LABEL);
const QString FoilAuthToken::KEY_SECRET(FOILAUTH_KEY_SECRET);
const QString FoilAuthToken::KEY_ISSUER(FOILAUTH_KEY_ISSUER);
const QString FoilAuthToken::KEY_DIGITS(FOILAUTH_KEY_DIGITS);
const QString FoilAuthToken::KEY_COUNTER(FOILAUTH_KEY_COUNTER);
const QString FoilAuthToken::KEY_TIMESHIFT(FOILAUTH_KEY_TIMESHIFT);
const QString FoilAuthToken::KEY_ALGORITHM(FOILAUTH_KEY_ALGORITHM);

const QString FoilAuthToken::TYPE_TOTP(FOILAUTH_TYPE_TOTP);
const QString FoilAuthToken::TYPE_HOTP(FOILAUTH_TYPE_HOTP);

const QString FoilAuthToken::ALGORITHM_SHA1(FOILAUTH_ALGORITHM_SHA1);
const QString FoilAuthToken::ALGORITHM_SHA256(FOILAUTH_ALGORITHM_SHA256);
const QString FoilAuthToken::ALGORITHM_SHA512(FOILAUTH_ALGORITHM_SHA512);

#define FOILAUTH_SCHEME "otpauth"

// ==========================================================================
// FoilAuthToken::Private
// ==========================================================================

// otpauth-migration payload
//
// message MigrationPayload {
// enum Algorithm {
//   ALGORITHM_UNSPECIFIED = 0;
//   ALGORITHM_SHA1 = 1;
//   ALGORITHM_SHA256 = 2;
//   ALGORITHM_SHA512 = 3;
//   ALGORITHM_MD5 = 4;  really???
// }
//
// enum DigitCount {
//   DIGIT_COUNT_UNSPECIFIED = 0;
//   DIGIT_COUNT_SIX = 1;
//   DIGIT_COUNT_EIGHT = 2;
// }
//
// enum OtpType {
//   OTP_TYPE_UNSPECIFIED = 0;
//   OTP_TYPE_HOTP = 1;
//   OTP_TYPE_TOTP = 2;
// }
//
// message OtpParameters {
//   bytes secret = 1;
//   string name = 2;
//   string issuer = 3;
//   Algorithm algorithm = 4;
//   DigitCount digits = 5;
//   OtpType type = 6;
//   int64 counter = 7;
// }
//
// repeated OtpParameters otp_parameters = 1;
// int32 version = 2;
// int32 batch_size = 3;
// int32 batch_index = 4;
// int32 batch_id = 5;
// }

class FoilAuthToken::Private
{
public:
    Private(AuthType, QByteArray, QString, QString, QString, int, quint64, int, DigestAlgorithm);
    ~Private();

    enum Algorithm {
        ALGORITHM_UNSPECIFIED,
        ALGORITHM_SHA1,
        ALGORITHM_SHA256,
        ALGORITHM_SHA512,
        ALGORITHM_MD5
    };

    enum DigitCount {
        DIGIT_COUNT_UNSPECIFIED,
        DIGIT_COUNT_SIX,
        DIGIT_COUNT_EIGHT,
    };

    enum OtpType {
        OTP_TYPE_UNSPECIFIED,
        OTP_TYPE_HOTP,
        OTP_TYPE_TOTP
    };

#define DELIMITED_TAG(x) (((x) << HarbourProtoBuf::TYPE_SHIFT) | HarbourProtoBuf::TYPE_DELIMITED)
#define VARINT_TAG(x) (((x) << HarbourProtoBuf::TYPE_SHIFT) | HarbourProtoBuf::TYPE_VARINT)

    static const uchar OTP_PARAMETERS_TAG = DELIMITED_TAG(1);
    static const uchar SECRET_TAG = DELIMITED_TAG(1);
    static const uchar NAME_TAG = DELIMITED_TAG(2);
    static const uchar ISSUER_TAG = DELIMITED_TAG(3);
    static const uchar ALGORITHM_TAG = VARINT_TAG(4);
    static const uchar DIGITS_TAG = VARINT_TAG(5);
    static const uchar TYPE_TAG = VARINT_TAG(6);
    static const uchar COUNTER_TAG = VARINT_TAG(7);

    static const uchar VERSION_TAG = VARINT_TAG(2);
    static const uchar BATCH_SIZE_TAG = VARINT_TAG(3);
    static const uchar BATCH_INDEX_TAG = VARINT_TAG(4);
    static const uchar BATCH_ID_TAG = VARINT_TAG(5);

    static const uchar VERSION = 1;

    struct OtpParameters {
        QByteArray secret;
        QString name;
        QString issuer;
        Algorithm algorithm;
        DigitCount digits;
        OtpType type;
        quint64 counter;

        OtpParameters() { clear(); }
        void clear() {
            secret.clear();
            name.clear();
            issuer.clear();
            algorithm = ALGORITHM_SHA1;
            digits = DIGIT_COUNT_SIX;
            type = OTP_TYPE_TOTP;
            counter = 0;
        }
        bool isValid() const {
            return !secret.isEmpty() &&
                (algorithm == ALGORITHM_SHA1 || algorithm == ALGORITHM_SHA256 ||
                 algorithm == ALGORITHM_SHA512) &&
                (digits == DIGIT_COUNT_SIX || digits == DIGIT_COUNT_EIGHT) &&
                (type == OTP_TYPE_TOTP || type == OTP_TYPE_HOTP);
        }
        static DigitCount encodeDigits(int aDigits) {
            switch (aDigits) {
            case 6: return DIGIT_COUNT_SIX;
            case 8: return DIGIT_COUNT_EIGHT;
            }
            return DIGIT_COUNT_UNSPECIFIED;
        }
        int numDigits() const {
            // Assume it's valid
            return (digits == DIGIT_COUNT_EIGHT) ? 8 : 6;
        }
        static Algorithm encodeAlgorithm(DigestAlgorithm aAlgorithm) {
            switch (aAlgorithm) {
            case DigestAlgorithmSHA1: return ALGORITHM_SHA1;
            case DigestAlgorithmSHA256: return ALGORITHM_SHA256;
            case DigestAlgorithmSHA512:  return ALGORITHM_SHA512;
            }
            return ALGORITHM_UNSPECIFIED;
        }
        DigestAlgorithm digestAlgorithm() const {
            switch (algorithm) {
            case ALGORITHM_SHA1: return DigestAlgorithmSHA1;
            case ALGORITHM_SHA256: return DigestAlgorithmSHA256;
            case ALGORITHM_SHA512:  return DigestAlgorithmSHA512;
            case ALGORITHM_MD5: // fallthrough
            case ALGORITHM_UNSPECIFIED: break;
            }
            return DEFAULT_ALGORITHM;
        }
        static OtpType encodeOtpType(AuthType aType) {
            switch (aType) {
            case AuthTypeHOTP: return OTP_TYPE_HOTP;
            case AuthTypeTOTP: return OTP_TYPE_TOTP;
            }
            return OTP_TYPE_UNSPECIFIED;
        }
        AuthType authType() const {
            switch (type) {
            case OTP_TYPE_HOTP: return AuthTypeHOTP;
            case OTP_TYPE_TOTP: return AuthTypeTOTP;
            case DEFAULT_AUTH_TYPE: break;
            }
            return DEFAULT_AUTH_TYPE;
        }
    };

    static bool parseOtpParameters(GUtilRange*, OtpParameters*);
    static void encodeTrailer(QByteArray*, uint, uint, quint64);

    uint password(quint64 aTime);

public:
    QAtomicInt iRef;
    AuthType iType;
    DigestAlgorithm iAlgorithm;
    QByteArray iSecret;
    QString iSecretBase32;
    QString iLabel;
    QString iIssuer;
    quint64 iCounter;
    int iDigits;
    int iTimeShift; // Seconds
};

FoilAuthToken::Private::Private(
    AuthType aType,
    QByteArray aSecret,
    QString aSecretBase32,
    QString aLabel,
    QString aIssuer,
    int aDigits,
    quint64 aCounter,
    int aTimeShift,
    DigestAlgorithm aAlgorithm) :
    iRef(1),
    iType(aType),
    iAlgorithm(aAlgorithm),
    iSecret(aSecret),
    iSecretBase32(aSecretBase32),
    iLabel(aLabel),
    iIssuer(aIssuer),
    iCounter(aCounter),
    iDigits(aDigits),
    iTimeShift(aTimeShift)
{
}

FoilAuthToken::Private::~Private()
{
}

uint
FoilAuthToken::Private::password(
    quint64 aTime)
{
    uint maxPass = 10;

    for (int i = 1; i < iDigits; i++) {
        maxPass *= 10;
    }

    return (iType == AuthTypeHOTP) ?
        FoilAuth::HOTP(iSecret, iCounter, maxPass, iAlgorithm) :
        FoilAuth::TOTP(iSecret, aTime, maxPass, iAlgorithm);
}

bool
FoilAuthToken::Private::parseOtpParameters(
    GUtilRange* aPos,
    OtpParameters* aResult)
{
    quint64 tag, value;
    GUtilRange pos = *aPos;
    while (HarbourProtoBuf::parseVarInt(&pos, &tag) &&
        (tag & HarbourProtoBuf::TYPE_MASK) == HarbourProtoBuf::TYPE_VARINT) {
        // Skip varints
        if (!HarbourProtoBuf::parseVarInt(&pos, &value)) {
            return false;
        }
    }

    // If parseVarInt failed, tag is zero
    GUtilData payload;
    if (tag == OTP_PARAMETERS_TAG && HarbourProtoBuf::parseDelimitedValue(&pos, &payload)) {
        // Looks like OtpParameters
        pos.ptr = payload.bytes;
        pos.end = payload.bytes + payload.size;
        OtpParameters params;
        quint64 value;
        while (HarbourProtoBuf::parseVarInt(&pos, &tag)) {
            switch (tag) {
            case SECRET_TAG:
                if (HarbourProtoBuf::parseDelimitedValue(&pos, &payload)) {
                    params.secret = QByteArray((const char*)payload.bytes, payload.size);
                } else {
                    return false;
                }
                break;
            case NAME_TAG:
                if (HarbourProtoBuf::parseDelimitedValue(&pos, &payload)) {
                    params.name = QString::fromUtf8((const char*)payload.bytes, payload.size);
                } else {
                    return false;
                }
                break;
            case ISSUER_TAG:
                if (HarbourProtoBuf::parseDelimitedValue(&pos, &payload)) {
                    params.issuer = QString::fromUtf8((const char*)payload.bytes, payload.size);
                } else {
                    return false;
                }
                break;
            case ALGORITHM_TAG:
                if (HarbourProtoBuf::parseVarInt(&pos, &value)) {
                    params.algorithm = (Algorithm)value;
                } else {
                    return false;
                }
                break;
            case DIGITS_TAG:
                if (HarbourProtoBuf::parseVarInt(&pos, &value)) {
                    params.digits = (DigitCount)value;
                } else {
                    return false;
                }
                break;
            case TYPE_TAG:
                if (HarbourProtoBuf::parseVarInt(&pos, &value)) {
                    params.type = (OtpType)value;
                } else {
                    return false;
                }
                break;
            case COUNTER_TAG:
                if (HarbourProtoBuf::parseVarInt(&pos, &value)) {
                    params.counter = value;
                } else {
                    return false;
                }
                break;
            default:
                // Something unintelligible
                return false;
            }
        }
        if (pos.ptr == pos.end && params.isValid()) {
            // The whole thing has been parsed
            aPos->ptr = pos.end;
            *aResult = params;
            return true;
        }
    }
    return false;
}

void
FoilAuthToken::Private::encodeTrailer(
    QByteArray* aOutput,
    uint aBatchIndex,
    uint aBatchSize,
    quint64 aBatchId)
{
    HarbourProtoBuf::appendVarIntKeyValue(aOutput, VERSION_TAG, VERSION);
    HarbourProtoBuf::appendVarIntKeyValue(aOutput, BATCH_SIZE_TAG, aBatchSize);
    HarbourProtoBuf::appendVarIntKeyValue(aOutput, BATCH_INDEX_TAG, aBatchIndex);
    HarbourProtoBuf::appendVarIntKeyValue(aOutput, BATCH_ID_TAG, aBatchId);
}

// ==========================================================================
// FoilAuthToken
// ==========================================================================

FoilAuthToken::FoilAuthToken() :
    iPrivate(NULL)
{
}

FoilAuthToken::FoilAuthToken(
    const FoilAuthToken& aToken) :
    iPrivate(aToken.iPrivate)
{
    if (iPrivate) {
        iPrivate->iRef.ref();
    }
}

FoilAuthToken::FoilAuthToken(
    AuthType aType,
    QByteArray aSecret,
    QString aLabel,
    QString aIssuer,
    int aDigits,
    quint64 aCounter,
    int aTimeShift,
    DigestAlgorithm aAlgorithm) :
    iPrivate(new Private(validType(aType),
        aSecret, HarbourBase32::toBase32(aSecret), aLabel, aIssuer,
        validDigits(aDigits), aCounter, aTimeShift, validAlgorithm(aAlgorithm)))
{
}

FoilAuthToken::FoilAuthToken(
    Private* aPrivate) : // Takes ownership
    iPrivate(aPrivate)
{
    HASSERT(!iPrivate || iPrivate->iRef.load() == 1);
}

FoilAuthToken::~FoilAuthToken()
{
    if (iPrivate && !iPrivate->iRef.deref()) {
        delete iPrivate;
    }
}

FoilAuthToken&
FoilAuthToken::operator=(
    const FoilAuthToken& aToken)
{
    if (iPrivate != aToken.iPrivate) {
        if (iPrivate && !iPrivate->iRef.deref()) {
            delete iPrivate;
        }
        iPrivate = aToken.iPrivate;
        if (iPrivate) {
            iPrivate->iRef.ref();
        }
    }
    return *this;
}

bool
FoilAuthToken::equals(
    const FoilAuthToken& aToken) const
{
    if (iPrivate == aToken.iPrivate) {
        return true;
    } else if (iPrivate && aToken.iPrivate) {
        const Private* other = aToken.iPrivate;

        return iPrivate->iType == other->iType &&
            iPrivate->iAlgorithm == other->iAlgorithm &&
            iPrivate->iDigits == other->iDigits &&
            iPrivate->iCounter == other->iCounter &&
            iPrivate->iTimeShift == other->iTimeShift &&
            iPrivate->iSecret == other->iSecret &&
            iPrivate->iLabel == other->iLabel &&
            iPrivate->iIssuer == other->iIssuer;
    } else {
        return false;
    }
}

bool
FoilAuthToken::isValid() const
{
    return iPrivate != Q_NULLPTR;
}

FoilAuthTypes::AuthType
FoilAuthToken::type() const
{
    return iPrivate ? iPrivate->iType : DEFAULT_AUTH_TYPE;
}

FoilAuthTypes::DigestAlgorithm
FoilAuthToken::algorithm() const
{
    return iPrivate ? iPrivate->iAlgorithm : DEFAULT_ALGORITHM;
}

const QString
FoilAuthToken::label() const
{
    return iPrivate ? iPrivate->iLabel : QString();
}

const QString
FoilAuthToken::issuer() const
{
    return iPrivate ? iPrivate->iIssuer: QString();
}

const QByteArray
FoilAuthToken::secret() const
{
    return iPrivate ? iPrivate->iSecret : QByteArray();
}

const QString
FoilAuthToken::secretBase32() const
{
    return iPrivate ? iPrivate->iSecretBase32 : QString();
}

quint64
FoilAuthToken::counter() const
{
    return iPrivate ? iPrivate->iCounter : 0;
}

int
FoilAuthToken::digits() const
{
    return iPrivate ? iPrivate->iDigits : 0;
}

int
FoilAuthToken::timeShift() const
{
    return iPrivate ? iPrivate->iTimeShift : 0;
}

uint
FoilAuthToken::password(
    quint64 aTime) const
{
    return iPrivate ? iPrivate->password(aTime) : 0;
}

QString
FoilAuthToken::passwordString(
    quint64 aTime) const
{
    return iPrivate ?
        QString().sprintf("%0*u", iPrivate->iDigits, password(aTime)) :
        QString();
}

int
FoilAuthToken::validDigits(
    int aDigits)
{
    return (aDigits >= MIN_DIGITS && aDigits <= MAX_DIGITS) ?
        aDigits : DEFAULT_DIGITS;
}

FoilAuthTypes::AuthType
FoilAuthToken::validType(
    AuthType aType)
{
    switch (aType) {
    case AuthTypeTOTP:
    case AuthTypeHOTP:
        return aType;
    }
    return DEFAULT_AUTH_TYPE;
}

FoilAuthTypes::DigestAlgorithm
FoilAuthToken::validAlgorithm(
    DigestAlgorithm aAlgorithm)
{
    switch (aAlgorithm) {
    case DigestAlgorithmSHA1:
    case DigestAlgorithmSHA256:
    case DigestAlgorithmSHA512:
        return aAlgorithm;
    }
    return DEFAULT_ALGORITHM;
}

FoilAuthToken
FoilAuthToken::fromUri(
    const QString aUri)
{
    const QByteArray uri(aUri.trimmed().toUtf8());

    GUtilRange pos;
    pos.ptr = (guint8*)uri.constData();
    pos.end = pos.ptr + uri.size();

    // Check scheme + type prefix
    GUtilData prefixBytes;
    AuthType type = AuthTypeTOTP;
    gutil_data_from_string(&prefixBytes, FOILAUTH_SCHEME "://"
        FOILAUTH_TYPE_TOTP "/");
    bool prefixOK = gutil_range_skip_prefix(&pos, &prefixBytes);
    if (!prefixOK) {
        type = AuthTypeHOTP;
        gutil_data_from_string(&prefixBytes, FOILAUTH_SCHEME "://"
            FOILAUTH_TYPE_HOTP "/");
        prefixOK = gutil_range_skip_prefix(&pos, &prefixBytes);
    }

    if (prefixOK) {
        QByteArray label, secret, issuer, algorithm, digits, counter;
        GUtilData secretTag, issuerTag, digitsTag, counterTag, algorithmTag;

        while (pos.ptr < pos.end && pos.ptr[0] != '?') {
            label.append(*pos.ptr++);
        }

        gutil_data_from_string(&secretTag, FOILAUTH_KEY_SECRET "=");
        gutil_data_from_string(&issuerTag, FOILAUTH_KEY_ISSUER "=");
        gutil_data_from_string(&digitsTag, FOILAUTH_KEY_DIGITS "=");
        gutil_data_from_string(&counterTag, FOILAUTH_KEY_COUNTER "=");
        gutil_data_from_string(&algorithmTag, FOILAUTH_KEY_ALGORITHM "=");

        while (pos.ptr < pos.end) {
            pos.ptr++;

            QByteArray* value =
                gutil_range_skip_prefix(&pos, &secretTag) ? &secret :
                gutil_range_skip_prefix(&pos, &issuerTag) ? &issuer :
                gutil_range_skip_prefix(&pos, &digitsTag) ? &digits :
                gutil_range_skip_prefix(&pos, &counterTag) ? &counter :
                gutil_range_skip_prefix(&pos, &algorithmTag) ? &algorithm :
                Q_NULLPTR;

            if (value) {
                *value = QByteArray();
                while (pos.ptr < pos.end && pos.ptr[0] != '&') {
                    value->append(*pos.ptr++);
                }
            } else {
                while (pos.ptr < pos.end && pos.ptr[0] != '&') {
                    pos.ptr++;
                }
            }
        }

        if (!secret.isEmpty()) {
            const QByteArray bytes(HarbourBase32::fromBase32(QUrl::fromPercentEncoding(secret)));

            if (!bytes.isEmpty()) {
                DigestAlgorithm alg = DEFAULT_ALGORITHM;
                int dig = DEFAULT_DIGITS;
                int imf = DEFAULT_COUNTER;
                int timeShift = DEFAULT_TIMESHIFT;

                if (!digits.isEmpty()) {
                    bool ok;
                    const int n = digits.toInt(&ok);
                    if (ok && n >=  MIN_DIGITS && n <= MAX_DIGITS) {
                        dig = n;
                    }
                }
                if (!counter.isEmpty()) {
                    bool ok;
                    const quint64 n = counter.toULongLong(&ok);
                    if (ok) {
                        imf = n;
                    }
                }
                if (!algorithm.isEmpty()) {
                    const QString algValue(QString::fromLatin1(algorithm).toUpper());
                    if (algValue == ALGORITHM_SHA1) {
                        alg = DigestAlgorithmSHA1;
                    } else if (algValue == ALGORITHM_SHA256) {
                        alg = DigestAlgorithmSHA256;
                    } else if (algValue == ALGORITHM_SHA512) {
                        alg = DigestAlgorithmSHA512;
                    }
                }
                return FoilAuthToken(type, bytes,
                    QUrl::fromPercentEncoding(label),
                    QUrl::fromPercentEncoding(issuer),
                    dig, imf, timeShift, alg);
            }
        }
    }
    return FoilAuthToken();
}

QString
FoilAuthToken::toUri() const
{
    if (isValid()) {
        QString buf(FOILAUTH_SCHEME "://");
        buf.append(iPrivate->iType == AuthTypeHOTP ? TYPE_HOTP : TYPE_TOTP);
        buf.append(QChar('/'));
        buf.append(QUrl::toPercentEncoding(iPrivate->iLabel, "@"));
        buf.append("?" FOILAUTH_KEY_SECRET "=");
        buf.append(iPrivate->iSecretBase32);
        if (!iPrivate->iIssuer.isEmpty()) {
            buf.append("&" FOILAUTH_KEY_ISSUER "=");
            buf.append(QUrl::toPercentEncoding(iPrivate->iIssuer, "@"));
        }
        buf.append("&" FOILAUTH_KEY_DIGITS "=");
        buf.append(QString::number(iPrivate->iDigits));
        switch (iPrivate->iAlgorithm) {
        case DigestAlgorithmSHA256:
            buf.append("&" FOILAUTH_KEY_ALGORITHM "=" FOILAUTH_ALGORITHM_SHA256);
            break;
        case DigestAlgorithmSHA512:
            buf.append("&" FOILAUTH_KEY_ALGORITHM "=" FOILAUTH_ALGORITHM_SHA512);
            break;
        case DEFAULT_ALGORITHM: // DigestAlgorithmSHA1
            break;
        }
        switch (iPrivate->iType) {
        case AuthTypeTOTP:
            if (iPrivate->iTimeShift != DEFAULT_TIMESHIFT) {
                buf.append("&" FOILAUTH_KEY_TIMESHIFT "=");
                buf.append(QString::number(iPrivate->iTimeShift));
            }
            break;
        case AuthTypeHOTP:
            buf.append("&" FOILAUTH_KEY_COUNTER "=");
            buf.append(QString::number(iPrivate->iCounter));
            break;
        }
        return buf;
    }
    return QString();
}

QVariantMap
FoilAuthToken::toVariantMap() const
{
    QVariantMap out;
    const bool valid = isValid();

    out.insert(KEY_VALID, valid);
    if (valid) {
        out.insert(KEY_TYPE, (int) iPrivate->iType);
        out.insert(KEY_LABEL, iPrivate->iLabel);
        out.insert(KEY_SECRET, iPrivate->iSecretBase32);
        out.insert(KEY_ISSUER, iPrivate->iIssuer);
        out.insert(KEY_DIGITS, iPrivate->iDigits);
        out.insert(KEY_COUNTER, iPrivate->iCounter);
        out.insert(KEY_TIMESHIFT, iPrivate->iTimeShift);
        out.insert(KEY_ALGORITHM, (int) iPrivate->iAlgorithm);
    }
    return out;
}


QList<FoilAuthToken>
FoilAuthToken::fromProtoBuf(
    const QByteArray& aData)
{
    GUtilRange pos;
    pos.ptr = (const guint8*) aData.constData();
    pos.end = pos.ptr + aData.size();

    QList<FoilAuthToken> result;
    Private::OtpParameters p;
    while (Private::parseOtpParameters(&pos, &p)) {
        if (p.isValid()) {
            result.append(FoilAuthToken(p.authType(), p.secret, p.name, p.issuer,
                p.numDigits(), p.counter, DEFAULT_TIMESHIFT, p.digestAlgorithm()));
        }
        p.clear();
    }
    return result;
}

QByteArray
FoilAuthToken::toProtoBuf() const
{
    QByteArray protobuf;
    if (iPrivate) {
        QByteArray payload;

        HarbourProtoBuf::appendDelimitedKeyValue(&payload,
            Private::SECRET_TAG,
            iPrivate->iSecret);
        HarbourProtoBuf::appendDelimitedKeyValue(&payload,
            Private::NAME_TAG,
            iPrivate->iLabel.toUtf8());
        if (!iPrivate->iIssuer.isEmpty()) {
            HarbourProtoBuf::appendDelimitedKeyValue(&payload,
                Private::ISSUER_TAG,
                iPrivate->iIssuer.toUtf8());
        }
        HarbourProtoBuf::appendVarIntKeyValue(&payload,
            Private::ALGORITHM_TAG,
            Private::OtpParameters::encodeAlgorithm(iPrivate->iAlgorithm));
        HarbourProtoBuf::appendVarIntKeyValue(&payload,
            Private::DIGITS_TAG,
            Private::OtpParameters::encodeDigits(iPrivate->iDigits));
        HarbourProtoBuf::appendVarIntKeyValue(&payload,
            Private::TYPE_TAG,
            Private::OtpParameters::encodeOtpType(iPrivate->iType));
        if (iPrivate->iType == AuthTypeHOTP) {
            HarbourProtoBuf::appendVarIntKeyValue(&payload,
                Private::COUNTER_TAG,
                iPrivate->iCounter);
        }

        protobuf.reserve(payload.size() + 2);
        HarbourProtoBuf::appendDelimitedKeyValue(&protobuf,
            Private::OTP_PARAMETERS_TAG,
            payload);
    }
    return protobuf;
}

QByteArray
FoilAuthToken::toProtoBuf(
    const QList<FoilAuthToken>& aTokens)
{
    QByteArray output;
    const int n = aTokens.count();
    for (int i = 0; i < n; i++) {
        output.append(aTokens.at(i).toProtoBuf());
    }
    // Trailer
    quint64 batchId;
    foil_random(&batchId, sizeof(batchId));
    Private::encodeTrailer(&output, 0, 1, batchId);
    return output;
}

//
// Note that the actual amount of information which end up
// in the QR code will be greater than aPrefBatchSize because
// of otpauth-migration://offline?data= prefix, BASE64 and URI
// encoding. Meaning that it's not a big deal if we exceed
// aPrefBatchSize by a few bytes.
//
// Tokens exceeding aMaxBatchSize are skipped.
//
QList<QByteArray>
FoilAuthToken::toProtoBufs(
    const QList<FoilAuthToken>& aTokens,
    int aPrefBatchSize,
    int aMaxBatchSize)
{
    QList<QByteArray> result;
    const int n = aTokens.count();

    HDEBUG(n << "token(s)");
    if (n > 0) {
        // Generate batch id
        quint64 batchId;
        foil_random(&batchId, sizeof(batchId));

        // Estimate the maximum size of the trailer
        QByteArray buf;
        Private::encodeTrailer(&buf, n - 1, n, batchId);
        const int maxTrailerSize = buf.size();

        int i;
        buf.resize(0);
        for (i = 0; i < n; i++) {
            const QByteArray tokenBuf(aTokens.at(i).toProtoBuf());
            if (buf.size() + tokenBuf.size() + maxTrailerSize < aPrefBatchSize) {
                buf.append(tokenBuf);
                // Can't add the trailer just yet, the batch size is unknown
            } else if (tokenBuf.size() + maxTrailerSize < aMaxBatchSize) {
                // Start the next buffer
                if (buf.size() > 0) {
                    result.append(buf);
                    buf = tokenBuf;
                }
            } else {
                // This token doesn't fit at all, skip it
                HDEBUG("Skipping token" << aTokens.at(i).label());
            }
        }

        if (buf.size() > 0) {
            result.append(buf);
        }

        // Add the trailer
        const int batchSize = result.count();
        HDEBUG("Batch size" << batchSize);
        for (i = 0; i < batchSize; i++) {
            Private::encodeTrailer(&result[i], i, batchSize, batchId);
        }
    }
    return result;
}

FoilAuthToken
FoilAuthToken::withType(
    AuthType aType) const
{

    if (iPrivate) {
        const AuthType type = validType(aType);

        if (iPrivate->iType != type) {
            return FoilAuthToken(new Private(type,
                iPrivate->iSecret, iPrivate->iSecretBase32, iPrivate->iLabel,
                iPrivate->iIssuer, iPrivate->iDigits, iPrivate->iCounter,
                iPrivate->iTimeShift, iPrivate->iAlgorithm));
        }
    }
    return *this;
}

FoilAuthToken
FoilAuthToken::withAlgorithm(
    DigestAlgorithm aAlgorithm) const
{
    if (iPrivate) {
        const DigestAlgorithm alg = validAlgorithm(aAlgorithm);

        if (iPrivate->iAlgorithm != alg) {
            return FoilAuthToken(new Private(iPrivate->iType,
                iPrivate->iSecret, iPrivate->iSecretBase32, iPrivate->iLabel,
                iPrivate->iIssuer, iPrivate->iDigits, iPrivate->iCounter,
                iPrivate->iTimeShift, alg));
        }
    }
    return *this;
}

FoilAuthToken
FoilAuthToken::withSecret(
    const QByteArray aSecret) const
{
    if (iPrivate && iPrivate->iSecret != aSecret) {
        return FoilAuthToken(new Private(iPrivate->iType,
            aSecret, HarbourBase32::toBase32(aSecret), iPrivate->iLabel,
            iPrivate->iIssuer, iPrivate->iDigits, iPrivate->iCounter,
            iPrivate->iTimeShift, iPrivate->iAlgorithm));
    }
    return *this;
}

FoilAuthToken
FoilAuthToken::withLabel(
    const QString aLabel) const
{
    if (iPrivate && iPrivate->iLabel != aLabel) {
        return FoilAuthToken(new Private(iPrivate->iType,
            iPrivate->iSecret, iPrivate->iSecretBase32, aLabel,
            iPrivate->iIssuer, iPrivate->iDigits, iPrivate->iCounter,
            iPrivate->iTimeShift, iPrivate->iAlgorithm));
    }
    return *this;
}

FoilAuthToken
FoilAuthToken::withIssuer(
    const QString aIssuer) const
{
    if (iPrivate && iPrivate->iIssuer != aIssuer) {
        return FoilAuthToken(new Private(iPrivate->iType,
            iPrivate->iSecret, iPrivate->iSecretBase32, iPrivate->iLabel,
            aIssuer, iPrivate->iDigits, iPrivate->iCounter,
            iPrivate->iTimeShift, iPrivate->iAlgorithm));
    }
    return *this;
}

FoilAuthToken
FoilAuthToken::withDigits(
    int aDigits) const
{
    if (iPrivate && iPrivate->iDigits != aDigits &&
        aDigits >= MIN_DIGITS && aDigits <= MAX_DIGITS) {
        return FoilAuthToken(new Private(iPrivate->iType,
            iPrivate->iSecret, iPrivate->iSecretBase32, iPrivate->iLabel,
            iPrivate->iIssuer, aDigits, iPrivate->iCounter,
            iPrivate->iTimeShift, iPrivate->iAlgorithm));
    }
    return *this;
}

FoilAuthToken
FoilAuthToken::withCounter(
    quint64 aCounter) const
{
    if (iPrivate && iPrivate->iCounter != aCounter) {
        return FoilAuthToken(new Private(iPrivate->iType,
            iPrivate->iSecret, iPrivate->iSecretBase32, iPrivate->iLabel,
            iPrivate->iIssuer, iPrivate->iDigits, aCounter,
            iPrivate->iTimeShift, iPrivate->iAlgorithm));
    }
    return *this;
}

FoilAuthToken
FoilAuthToken::withTimeShift(
    int aTimeShift) const
{
    if (iPrivate && iPrivate->iTimeShift != aTimeShift) {
        return FoilAuthToken(new Private(iPrivate->iType,
            iPrivate->iSecret, iPrivate->iSecretBase32, iPrivate->iLabel,
            iPrivate->iIssuer, iPrivate->iDigits, iPrivate->iCounter,
            aTimeShift, iPrivate->iAlgorithm));
    }
    return *this;
}

#if HARBOUR_DEBUG
QDebug
operator<<(
    QDebug aDebug,
    const FoilAuthToken& aToken)
{
    if (aToken.isValid()) {
        QDebugStateSaver saver(aDebug);
        aDebug.nospace() << "FoilAuthToken(" << aToken.label() <<
            ", " << aToken.issuer() << ", " <<aToken.type() <<
            ", " << aToken.algorithm() << ", " <<  aToken.secretBase32() <<
            ", " << aToken.digits() << ", " << aToken.counter() << ")";
    } else {
        aDebug << "FoilAuthToken()";
    }
    return aDebug;
}
#endif // HARBOUR_DEBUG
