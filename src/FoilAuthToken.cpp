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

#include "FoilAuthToken.h"
#include "FoilAuth.h"

#include "qrencode.h"

#include <foil_util.h>
#include <foil_random.h>

#include <QUrl>

#define FOILAUTH_KEY_TYPE "type"
#define FOILAUTH_KEY_LABEL "label"
#define FOILAUTH_KEY_SECRET "secret"
#define FOILAUTH_KEY_ISSUER "issuer"
#define FOILAUTH_KEY_DIGITS "digits"
#define FOILAUTH_KEY_ALGORITHM "algorithm"
#define FOILAUTH_KEY_TIMESHIFT "timeshift"

const QString FoilAuthToken::KEY_VALID("valid");
const QString FoilAuthToken::KEY_TYPE(FOILAUTH_KEY_TYPE);
const QString FoilAuthToken::KEY_LABEL(FOILAUTH_KEY_LABEL);
const QString FoilAuthToken::KEY_SECRET(FOILAUTH_KEY_SECRET);
const QString FoilAuthToken::KEY_ISSUER(FOILAUTH_KEY_ISSUER);
const QString FoilAuthToken::KEY_DIGITS(FOILAUTH_KEY_DIGITS);
const QString FoilAuthToken::KEY_ALGORITHM(FOILAUTH_KEY_ALGORITHM);
const QString FoilAuthToken::KEY_TIMESHIFT(FOILAUTH_KEY_TIMESHIFT);

#define FOILAUTH_TYPE_TOTP "totp"
#define FOILAUTH_TYPE_HOTP "hotp"

const QString FoilAuthToken::TYPE_TOTP(FOILAUTH_TYPE_TOTP);
const QString FoilAuthToken::TYPE_HOTP(FOILAUTH_TYPE_HOTP);

const QString FoilAuthToken::ALGORITHM_MD5(FOILAUTH_ALGORITHM_MD5);
const QString FoilAuthToken::ALGORITHM_SHA1(FOILAUTH_ALGORITHM_SHA1);
const QString FoilAuthToken::ALGORITHM_SHA256(FOILAUTH_ALGORITHM_SHA256);
const QString FoilAuthToken::ALGORITHM_SHA512(FOILAUTH_ALGORITHM_SHA512);

#define FOILAUTH_SCHEME "otpauth"

FoilAuthToken::FoilAuthToken() :
    iAlgorithm(DEFAULT_ALGORITHM),
    iDigits(DEFAULT_DIGITS),
    iTimeShift(DEFAULT_TIMESHIFT)
{
}

FoilAuthToken::FoilAuthToken(const FoilAuthToken& aToken) :
    iAlgorithm(aToken.iAlgorithm),
    iBytes(aToken.iBytes),
    iLabel(aToken.iLabel),
    iIssuer(aToken.iIssuer),
    iDigits(aToken.iDigits),
    iTimeShift(aToken.iTimeShift)
{
}

FoilAuthToken::FoilAuthToken(QByteArray aBytes, QString aLabel,
    QString aIssuer, int aDigits, int aTimeShift, DigestAlgorithm aAlgorithm) :
    iAlgorithm(DEFAULT_ALGORITHM),
    iBytes(aBytes),
    iLabel(aLabel),
    iIssuer(aIssuer),
    iDigits(DEFAULT_DIGITS),
    iTimeShift(aTimeShift)
{
    setDigits(aDigits);
    setAlgorithm(aAlgorithm);
}

FoilAuthToken& FoilAuthToken::operator=(const FoilAuthToken& aToken)
{
    iAlgorithm = aToken.iAlgorithm;
    iBytes = aToken.iBytes;
    iLabel = aToken.iLabel;
    iIssuer = aToken.iIssuer;
    iDigits = aToken.iDigits;
    iTimeShift = aToken.iTimeShift;
    return *this;
}

bool FoilAuthToken::equals(const FoilAuthToken& aToken) const
{
    return iAlgorithm == aToken.iAlgorithm &&
        iDigits == aToken.iDigits &&
        iTimeShift == aToken.iTimeShift &&
        iBytes == aToken.iBytes &&
        iLabel == aToken.iLabel &&
        iIssuer == aToken.iIssuer;
}

uint FoilAuthToken::password(quint64 aTime) const
{
    uint maxPass = 10;
    for (int i = 1; i < iDigits; i++) {
        maxPass *= 10;
    }
    return FoilAuth::TOTP(iBytes, aTime, maxPass, iAlgorithm);
}

QString FoilAuthToken::passwordString(quint64 aTime) const
{
    return QString().sprintf("%0*u", iDigits, password(aTime));
}

bool FoilAuthToken::setDigits(int aDigits)
{
    if (aDigits >= 1 && aDigits <= MAX_DIGITS) {
        iDigits = aDigits;
        return true;
    }
    return false;
}

bool FoilAuthToken::setAlgorithm(DigestAlgorithm aAlgorithm)
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

bool FoilAuthToken::parseUri(QString aUri)
{
    const QByteArray uri(aUri.trimmed().toUtf8());

    FoilParsePos pos;
    pos.ptr = (guint8*)uri.constData();
    pos.end = pos.ptr + uri.size();

    FoilBytes prefixBytes;
    foil_bytes_from_string(&prefixBytes, FOILAUTH_SCHEME "://"
        FOILAUTH_TYPE_TOTP "/");
    if (foil_parse_skip_bytes(&pos, &prefixBytes)) {
        QByteArray label, secret, issuer, algorithm, digits;
        while (pos.ptr < pos.end && pos.ptr[0] != '?') {
            label.append(*pos.ptr++);
        }

        FoilBytes secretTag;
        FoilBytes issuerTag;
        FoilBytes digitsTag;
        FoilBytes algorithmTag;
        foil_bytes_from_string(&secretTag, FOILAUTH_KEY_SECRET "=");
        foil_bytes_from_string(&issuerTag, FOILAUTH_KEY_ISSUER "=");
        foil_bytes_from_string(&digitsTag, FOILAUTH_KEY_DIGITS "=");
        foil_bytes_from_string(&algorithmTag, FOILAUTH_KEY_ALGORITHM "=");
        QByteArray* value;

        while (pos.ptr < pos.end) {
            pos.ptr++;
            if (foil_parse_skip_bytes(&pos, &secretTag)) {
                value = &secret;
            } else if (foil_parse_skip_bytes(&pos, &issuerTag)) {
                value = &issuer;
            } else if (foil_parse_skip_bytes(&pos, &digitsTag)) {
                value = &digits;
            } else if (foil_parse_skip_bytes(&pos, &algorithmTag)) {
                value = &algorithm;
            } else {
                value = Q_NULLPTR;
            }
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
                iBytes = bytes;
                iLabel = QUrl::fromPercentEncoding(label);
                iIssuer = QUrl::fromPercentEncoding(issuer);
                iDigits = DEFAULT_DIGITS;
                iAlgorithm = DEFAULT_ALGORITHM;
                iTimeShift = DEFAULT_TIMESHIFT;

                if (!digits.isEmpty()) {
                    const int n = digits.toInt();
                    if (n >=  MIN_DIGITS && n <= MAX_DIGITS) {
                        iDigits = n;
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

QString FoilAuthToken::toUri() const
{
    if (isValid()) {
        QString buf(FOILAUTH_SCHEME "://");
        buf.append(TYPE_TOTP);
        buf.append(QChar('/'));
        buf.append(QUrl::toPercentEncoding(iLabel, " @"));
        buf.append("?" FOILAUTH_KEY_SECRET "=");
        buf.append(FoilAuth::toBase32(iBytes));
        if (!iIssuer.isEmpty()) {
            buf.append("&" FOILAUTH_KEY_ISSUER "=");
            buf.append(QUrl::toPercentEncoding(iIssuer, " @"));
        }
        buf.append("&" FOILAUTH_KEY_DIGITS "=");
        buf.append(QString::number(iDigits));
        if (iAlgorithm != DEFAULT_ALGORITHM) {
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
        }
        return buf;
    }
    return QString();
}

QVariantMap FoilAuthToken::toVariantMap() const
{
    QVariantMap out;
    const bool valid = isValid();
    out.insert(KEY_VALID, valid);
    if (valid) {
        out.insert(KEY_TYPE, TYPE_TOTP);
        out.insert(KEY_LABEL, iLabel);
        out.insert(KEY_SECRET, FoilAuth::toBase32(iBytes));
        out.insert(KEY_ISSUER, iIssuer);
        out.insert(KEY_DIGITS, iDigits);
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

class FoilAuthToken::Private {
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

    static const uchar PROTOBUF_TYPE_SHIFT = 3;
    static const uchar PROTOBUF_TYPE_MASK = ((1 << PROTOBUF_TYPE_SHIFT)-1);
    static const uchar PROTOBUF_TYPE_VARINT = 0;
    static const uchar PROTOBUF_TYPE_BYTES = 2;

#define BYTES_TAG(x) (((x) << PROTOBUF_TYPE_SHIFT) | PROTOBUF_TYPE_BYTES)
#define VARINT_TAG(x) (((x) << PROTOBUF_TYPE_SHIFT) | PROTOBUF_TYPE_VARINT)

    static const uchar OTP_PARAMETERS_TAG = BYTES_TAG(1);
    static const uchar SECRET_TAG = BYTES_TAG(1);
    static const uchar NAME_TAG = BYTES_TAG(2);
    static const uchar ISSUER_TAG = BYTES_TAG(3);
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
                type == OTP_TYPE_TOTP;
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
    };

    static bool parseVarInt(FoilParsePos* aPos, quint64* aResult);
    static bool parsePayload(FoilParsePos* aPos, FoilBytes* aResult);
    static bool parseOtpParameters(FoilParsePos* aPos, OtpParameters* aResult);
    static void encodeVarInt(quint64 aValue, QByteArray* aOutput);
    static QByteArray encodeBytes(const QByteArray aBytes);
};

bool FoilAuthToken::Private::parseVarInt(FoilParsePos* aPos, quint64* aResult)
{
    quint64 value = 0;
    const guint8* ptr = aPos->ptr;
    for (int i = 0; i < 10 && ptr < aPos->end; i++, ptr++) {
        value = (value << 7) | (*ptr & 0x7f);
        if (!(*ptr & 0x80)) {
            aPos->ptr = ptr + 1;
            *aResult = value;
            return true;
        }
    }
    // Premature end of stream or too many bytes
    *aResult = 0;
    return false;
}

void FoilAuthToken::Private::encodeVarInt(quint64 aValue, QByteArray* aOutput)
{
    uchar out[10];
    quint64 value = aValue;
    int i = sizeof(out) - 1;
    out[i] = value & 0x7f;
    value >>= 7;
    while (value) {
        out[--i] = 0x80 | (uchar)value;
        value >>= 7;
    }
    const int n = sizeof(out) - i;
    aOutput->reserve(aOutput->size() + n);
    aOutput->append((char*)(out + i), n);
}

bool FoilAuthToken::Private::parsePayload(FoilParsePos* aPos, FoilBytes* aResult)
{
    // Varint size followed by payload bytes
    FoilParsePos pos = *aPos;
    quint64 size;
    if (parseVarInt(&pos, &size) && (pos.ptr + size) <= pos.end) {
        aResult->val = pos.ptr;
        aResult->len = size;
        aPos->ptr = pos.ptr + size;
        return true;
    }
    return false;
}

QByteArray FoilAuthToken::Private::encodeBytes(const QByteArray aBytes)
{
    QByteArray output;
    encodeVarInt(aBytes.size(), &output);
    output.append(aBytes);
    return output;
}

bool FoilAuthToken::Private::parseOtpParameters(FoilParsePos* aPos, OtpParameters* aResult)
{
    quint64 tag, value;
    FoilParsePos pos = *aPos;
    while (parseVarInt(&pos, &tag) && (tag & PROTOBUF_TYPE_MASK) == PROTOBUF_TYPE_VARINT) {
        // Skip varints
        if (!parseVarInt(&pos, &value)) {
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
        while (parseVarInt(&pos, &tag)) {
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
                if (parseVarInt(&pos, &value)) {
                    params.algorithm = (Algorithm)value;
                } else {
                    return false;
                }
                break;
            case DIGITS_TAG:
                if (parseVarInt(&pos, &value)) {
                    params.digits = (DigitCount)value;
                } else {
                    return false;
                }
                break;
            case TYPE_TAG:
                if (parseVarInt(&pos, &value)) {
                    params.type = (OtpType)value;
                } else {
                    return false;
                }
                break;
            case COUNTER_TAG:
                if (!parseVarInt(&pos, &value)) {
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

QList<FoilAuthToken> FoilAuthToken::fromProtoBuf(const QByteArray& aData)
{
    FoilParsePos pos;
    pos.ptr = (const guint8*) aData.constData();
    pos.end = pos.ptr + aData.size();

    QList<FoilAuthToken> result;
    Private::OtpParameters p;
    while (Private::parseOtpParameters(&pos, &p)) {
        if (p.isValid()) {
            result.append(FoilAuthToken(p.secret, p.name, p.issuer, p.numDigits(),
                DEFAULT_TIMESHIFT, p.digestAlgorithm()));
        }
        p.clear();
    }
    return result;
}

QByteArray FoilAuthToken::toProtoBuf() const
{
    QByteArray payload;
    payload.append(Private::SECRET_TAG);
    payload.append(Private::encodeBytes(iBytes));
    payload.append(Private::NAME_TAG);
    payload.append(Private::encodeBytes(iLabel.toUtf8()));
    payload.append(Private::ISSUER_TAG);
    payload.append(Private::encodeBytes(iIssuer.toUtf8()));
    payload.append(Private::ALGORITHM_TAG);
    Private::encodeVarInt(Private::OtpParameters::encodeAlgorithm(iAlgorithm), &payload);
    payload.append(Private::DIGITS_TAG);
    Private::encodeVarInt(Private::OtpParameters::encodeDigits(iDigits), &payload);
    payload.append(Private::TYPE_TAG);
    Private::encodeVarInt(Private::OTP_TYPE_TOTP, &payload);
    payload.append(Private::COUNTER_TAG);
    Private::encodeVarInt(0, &payload);

    QByteArray protobuf;
    protobuf.reserve(payload.size() + 2);
    protobuf.append(Private::OTP_PARAMETERS_TAG);
    protobuf.append(Private::encodeBytes(payload));
    return protobuf;
}

QByteArray FoilAuthToken::toProtoBuf(const QList<FoilAuthToken>& aTokens)
{
    QByteArray output;
    for (int i = 0; i < aTokens.count(); i++) {
        output.append(aTokens.at(i).toProtoBuf());
    }
    // Trailer
    output.append(Private::VERSION_TAG);
    Private::encodeVarInt(Private::VERSION, &output);
    output.append(Private::BATCH_SIZE_TAG);
    Private::encodeVarInt(1, &output);
    output.append(Private::BATCH_INDEX_TAG);
    Private::encodeVarInt(0, &output);
    quint64 batch_id = 0;
    foil_random(&batch_id, sizeof(batch_id));
    output.append(Private::BATCH_ID_TAG);
    Private::encodeVarInt(batch_id, &output);
    return output;
}
