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

#include "HarbourDebug.h"
#include "HarbourProtoBuf.h"

#include "qrencode.h"

#include <foil_util.h>
#include <foil_random.h>

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

const QString FoilAuthToken::ALGORITHM_MD5(FOILAUTH_ALGORITHM_MD5);
const QString FoilAuthToken::ALGORITHM_SHA1(FOILAUTH_ALGORITHM_SHA1);
const QString FoilAuthToken::ALGORITHM_SHA256(FOILAUTH_ALGORITHM_SHA256);
const QString FoilAuthToken::ALGORITHM_SHA512(FOILAUTH_ALGORITHM_SHA512);

#define FOILAUTH_SCHEME "otpauth"

FoilAuthToken::FoilAuthToken() :
    iType(DEFAULT_AUTH_TYPE),
    iAlgorithm(DEFAULT_ALGORITHM),
    iCounter(DEFAULT_COUNTER),
    iDigits(DEFAULT_DIGITS),
    iTimeShift(DEFAULT_TIMESHIFT)
{
}

FoilAuthToken::FoilAuthToken(
    const FoilAuthToken& aToken) :
    iType(aToken.iType),
    iAlgorithm(aToken.iAlgorithm),
    iBytes(aToken.iBytes),
    iLabel(aToken.iLabel),
    iIssuer(aToken.iIssuer),
    iCounter(aToken.iCounter),
    iDigits(aToken.iDigits),
    iTimeShift(aToken.iTimeShift)
{
}

FoilAuthToken::FoilAuthToken(
    AuthType aType,
    QByteArray aBytes,
    QString aLabel,
    QString aIssuer,
    int aDigits,
    quint64 aCounter,
    int aTimeShift,
    DigestAlgorithm aAlgorithm) :
    iType(AuthTypeTOTP),
    iAlgorithm(DEFAULT_ALGORITHM),
    iBytes(aBytes),
    iLabel(aLabel),
    iIssuer(aIssuer),
    iCounter(aCounter),
    iDigits(DEFAULT_DIGITS),
    iTimeShift(aTimeShift)
{
    setType(aType);
    setDigits(aDigits);
    setAlgorithm(aAlgorithm);
}

FoilAuthToken&
FoilAuthToken::operator=(
    const FoilAuthToken& aToken)
{
    iType = aToken.iType;
    iAlgorithm = aToken.iAlgorithm;
    iBytes = aToken.iBytes;
    iLabel = aToken.iLabel;
    iIssuer = aToken.iIssuer;
    iCounter = aToken.iCounter;
    iDigits = aToken.iDigits;
    iTimeShift = aToken.iTimeShift;
    return *this;
}

bool
FoilAuthToken::equals(
    const FoilAuthToken& aToken) const
{
    return iType == aToken.iType &&
        iAlgorithm == aToken.iAlgorithm &&
        iDigits == aToken.iDigits &&
        iCounter == aToken.iCounter &&
        iTimeShift == aToken.iTimeShift &&
        iBytes == aToken.iBytes &&
        iLabel == aToken.iLabel &&
        iIssuer == aToken.iIssuer;
}

uint
FoilAuthToken::password(
    quint64 aTime) const
{
    uint maxPass = 10;
    for (int i = 1; i < iDigits; i++) {
        maxPass *= 10;
    }
    return (iType == AuthTypeHOTP) ?
        FoilAuth::HOTP(iBytes, iCounter, maxPass, iAlgorithm) :
        FoilAuth::TOTP(iBytes, aTime, maxPass, iAlgorithm);
}

QString
FoilAuthToken::passwordString(
    quint64 aTime) const
{
    return QString().sprintf("%0*u", iDigits, password(aTime));
}

bool
FoilAuthToken::setDigits(
    int aDigits)
{
    if (aDigits >= 1 && aDigits <= MAX_DIGITS) {
        iDigits = aDigits;
        return true;
    }
    return false;
}

bool
FoilAuthToken::setType(
    AuthType aType)
{
    switch (aType) {
    case AuthTypeTOTP:
    case AuthTypeHOTP:
        iType = aType;
        return true;
    }
    return false;
}

bool
FoilAuthToken::setAlgorithm(
    DigestAlgorithm aAlgorithm)
{
    switch (aAlgorithm) {
    case DigestAlgorithmMD5:
    case DigestAlgorithmSHA1:
    case DigestAlgorithmSHA256:
    case DigestAlgorithmSHA512:
        iAlgorithm = aAlgorithm;
        return true;
    }
    return false;
}

bool
FoilAuthToken::parseUri(
    QString aUri)
{
    const QByteArray uri(aUri.trimmed().toUtf8());

    GUtilRange pos;
    pos.ptr = (guint8*)uri.constData();
    pos.end = pos.ptr + uri.size();

    // Check scheme + type prefix
    FoilBytes prefixBytes;
    AuthType type = AuthTypeTOTP;
    foil_bytes_from_string(&prefixBytes, FOILAUTH_SCHEME "://"
        FOILAUTH_TYPE_TOTP "/");
    bool prefixOK = foil_parse_skip_bytes(&pos, &prefixBytes);
    if (!prefixOK) {
        type = AuthTypeHOTP;
        foil_bytes_from_string(&prefixBytes, FOILAUTH_SCHEME "://"
            FOILAUTH_TYPE_HOTP "/");
        prefixOK = foil_parse_skip_bytes(&pos, &prefixBytes);
    }

    if (prefixOK) {
        QByteArray label, secret, issuer, algorithm, digits, counter;
        while (pos.ptr < pos.end && pos.ptr[0] != '?') {
            label.append(*pos.ptr++);
        }

        FoilBytes secretTag;
        FoilBytes issuerTag;
        FoilBytes digitsTag;
        FoilBytes counterTag;
        FoilBytes algorithmTag;
        foil_bytes_from_string(&secretTag, FOILAUTH_KEY_SECRET "=");
        foil_bytes_from_string(&issuerTag, FOILAUTH_KEY_ISSUER "=");
        foil_bytes_from_string(&digitsTag, FOILAUTH_KEY_DIGITS "=");
        foil_bytes_from_string(&counterTag, FOILAUTH_KEY_COUNTER "=");
        foil_bytes_from_string(&algorithmTag, FOILAUTH_KEY_ALGORITHM "=");

        while (pos.ptr < pos.end) {
            pos.ptr++;

            QByteArray* value =
                foil_parse_skip_bytes(&pos, &secretTag) ? &secret :
                foil_parse_skip_bytes(&pos, &issuerTag) ? &issuer :
                foil_parse_skip_bytes(&pos, &digitsTag) ? &digits :
                foil_parse_skip_bytes(&pos, &counterTag) ? &counter :
                foil_parse_skip_bytes(&pos, &algorithmTag) ? &algorithm :
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
            QByteArray bytes = FoilAuth::fromBase32(QUrl::fromPercentEncoding(secret));
            if (!bytes.isEmpty()) {
                iType = type;
                iBytes = bytes;
                iLabel = QUrl::fromPercentEncoding(label);
                iIssuer = QUrl::fromPercentEncoding(issuer);
                iDigits = DEFAULT_DIGITS;
                iCounter = DEFAULT_COUNTER;
                iAlgorithm = DEFAULT_ALGORITHM;
                iTimeShift = DEFAULT_TIMESHIFT;

                if (!digits.isEmpty()) {
                    bool ok;
                    const int n = digits.toInt(&ok);
                    if (ok && n >=  MIN_DIGITS && n <= MAX_DIGITS) {
                        iDigits = n;
                    }
                }
                if (!counter.isEmpty()) {
                    bool ok;
                    const quint64 n = counter.toULongLong(&ok);
                    if (ok) {
                        iCounter = n;
                    }
                }
                if (!algorithm.isEmpty()) {
                    const QString alg(QString::fromLatin1(algorithm));
                    if (alg == ALGORITHM_MD5) {
                        iAlgorithm = DigestAlgorithmMD5;
                    } else if (alg == ALGORITHM_SHA1) {
                        iAlgorithm = DigestAlgorithmSHA1;
                    } else if (alg == ALGORITHM_SHA256) {
                        iAlgorithm = DigestAlgorithmSHA256;
                    } else if (alg == ALGORITHM_SHA512) {
                        iAlgorithm = DigestAlgorithmSHA512;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

QString
FoilAuthToken::toUri() const
{
    if (isValid()) {
        QString buf(FOILAUTH_SCHEME "://");
        buf.append(iType == AuthTypeHOTP ? TYPE_HOTP : TYPE_TOTP);
        buf.append(QChar('/'));
        buf.append(QUrl::toPercentEncoding(iLabel, "@"));
        buf.append("?" FOILAUTH_KEY_SECRET "=");
        buf.append(FoilAuth::toBase32(iBytes));
        if (!iIssuer.isEmpty()) {
            buf.append("&" FOILAUTH_KEY_ISSUER "=");
            buf.append(QUrl::toPercentEncoding(iIssuer, "@"));
        }
        buf.append("&" FOILAUTH_KEY_DIGITS "=");
        buf.append(QString::number(iDigits));
        switch (iAlgorithm) {
        case DigestAlgorithmMD5:
            buf.append("&" FOILAUTH_KEY_ALGORITHM "=" FOILAUTH_ALGORITHM_MD5);
            break;
        case DigestAlgorithmSHA256:
            buf.append("&" FOILAUTH_KEY_ALGORITHM "=" FOILAUTH_ALGORITHM_SHA256);
            break;
        case DigestAlgorithmSHA512:
            buf.append("&" FOILAUTH_KEY_ALGORITHM "=" FOILAUTH_ALGORITHM_SHA512);
            break;
        case DEFAULT_ALGORITHM: // DigestAlgorithmSHA1
            break;
        }
        switch (iType) {
        case AuthTypeTOTP:
            if (iTimeShift != DEFAULT_TIMESHIFT) {
                buf.append("&" FOILAUTH_KEY_TIMESHIFT "=");
                buf.append(QString::number(iTimeShift));
            }
            break;
        case AuthTypeHOTP:
            buf.append("&" FOILAUTH_KEY_COUNTER "=");
            buf.append(QString::number(iCounter));
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
        out.insert(KEY_TYPE, (int) iType);
        out.insert(KEY_LABEL, iLabel);
        out.insert(KEY_SECRET, FoilAuth::toBase32(iBytes));
        out.insert(KEY_ISSUER, iIssuer);
        out.insert(KEY_DIGITS, iDigits);
        out.insert(KEY_COUNTER, iCounter);
        out.insert(KEY_TIMESHIFT, iTimeShift);
        out.insert(KEY_ALGORITHM, (int) iAlgorithm);
    }
    return out;
}

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
                 algorithm == ALGORITHM_SHA512 || algorithm == ALGORITHM_MD5) &&
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
            case DigestAlgorithmMD5: return ALGORITHM_MD5;
            case DigestAlgorithmSHA1: return ALGORITHM_SHA1;
            case DigestAlgorithmSHA256: return ALGORITHM_SHA256;
            case DigestAlgorithmSHA512:  return ALGORITHM_SHA512;
            }
            return ALGORITHM_UNSPECIFIED;
        }
        DigestAlgorithm digestAlgorithm() const {
            switch (algorithm) {
            case ALGORITHM_MD5: return DigestAlgorithmMD5;
            case ALGORITHM_SHA1: return DigestAlgorithmSHA1;
            case ALGORITHM_SHA256: return DigestAlgorithmSHA256;
            case ALGORITHM_SHA512:  return DigestAlgorithmSHA512;
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

    static bool parsePayload(GUtilRange*, FoilBytes*);
    static bool parseOtpParameters(GUtilRange*, OtpParameters*);
    static void encodeTrailer(QByteArray*, uint, uint, quint64);
};

bool
FoilAuthToken::Private::parsePayload(
    GUtilRange* aPos,
    FoilBytes* aPayload)
{
    GUtilData payload;

    if (HarbourProtoBuf::parseDelimitedValue(aPos, &payload)) {
        aPayload->val = payload.bytes;
        aPayload->len = payload.size;
        return true;
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
    FoilBytes payload;
    if (tag == OTP_PARAMETERS_TAG && parsePayload(&pos, &payload)) {
        // Looks like OtpParameters
        pos.ptr = payload.val;
        pos.end = payload.val + payload.len;
        OtpParameters params;
        quint64 value;
        while (HarbourProtoBuf::parseVarInt(&pos, &tag)) {
            switch (tag) {
            case SECRET_TAG:
                if (parsePayload(&pos, &payload)) {
                    params.secret = QByteArray((const char*)payload.val, payload.len);
                } else {
                    return false;
                }
                break;
            case NAME_TAG:
                if (parsePayload(&pos, &payload)) {
                    params.name = QString::fromUtf8((const char*)payload.val, payload.len);
                } else {
                    return false;
                }
                break;
            case ISSUER_TAG:
                if (parsePayload(&pos, &payload)) {
                    params.issuer = QString::fromUtf8((const char*)payload.val, payload.len);
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
    QByteArray payload;
    HarbourProtoBuf::appendDelimitedKeyValue(&payload, Private::SECRET_TAG, iBytes);
    HarbourProtoBuf::appendDelimitedKeyValue(&payload, Private::NAME_TAG, iLabel.toUtf8());
    if (!iIssuer.isEmpty()) {
        HarbourProtoBuf::appendDelimitedKeyValue(&payload, Private::ISSUER_TAG, iIssuer.toUtf8());
    }
    HarbourProtoBuf::appendVarIntKeyValue(&payload, Private::ALGORITHM_TAG,
        Private::OtpParameters::encodeAlgorithm(iAlgorithm));
    HarbourProtoBuf::appendVarIntKeyValue(&payload, Private::DIGITS_TAG,
        Private::OtpParameters::encodeDigits(iDigits));
    HarbourProtoBuf::appendVarIntKeyValue(&payload, Private::TYPE_TAG,
        Private::OtpParameters::encodeOtpType(iType));
    if (iType == AuthTypeHOTP) {
        HarbourProtoBuf::appendVarIntKeyValue(&payload, Private::COUNTER_TAG, iCounter);
    }

    QByteArray protobuf;
    protobuf.reserve(payload.size() + 2);
    HarbourProtoBuf::appendDelimitedKeyValue(&protobuf, Private::OTP_PARAMETERS_TAG, payload);
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
                HDEBUG("Skipping token" << aTokens.at(i).iLabel);
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
