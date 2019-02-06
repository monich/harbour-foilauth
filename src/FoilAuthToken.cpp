/*
 * Copyright (C) 2019 Jolla Ltd.
 * Copyright (C) 2019 Slava Monich <slava@monich.com>
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

#include <QUrl>

#define FOILAUTH_KEY_TYPE "type"
#define FOILAUTH_KEY_LABEL "label"
#define FOILAUTH_KEY_SECRET "secret"
#define FOILAUTH_KEY_ISSUER "issuer"
#define FOILAUTH_KEY_DIGITS "digits"
#define FOILAUTH_KEY_TIMESHIFT "timeshift"

const QString FoilAuthToken::KEY_VALID("valid");
const QString FoilAuthToken::KEY_TYPE(FOILAUTH_KEY_TYPE);
const QString FoilAuthToken::KEY_LABEL(FOILAUTH_KEY_LABEL);
const QString FoilAuthToken::KEY_SECRET(FOILAUTH_KEY_SECRET);
const QString FoilAuthToken::KEY_ISSUER(FOILAUTH_KEY_ISSUER);
const QString FoilAuthToken::KEY_DIGITS(FOILAUTH_KEY_DIGITS);
const QString FoilAuthToken::KEY_TIMESHIFT(FOILAUTH_KEY_TIMESHIFT);

#define FOILAUTH_TYPE_TOTP "totp"
#define FOILAUTH_TYPE_HOTP "hotp"

const QString FoilAuthToken::TYPE_TOTP(FOILAUTH_TYPE_TOTP);
const QString FoilAuthToken::TYPE_HOTP(FOILAUTH_TYPE_HOTP);

#define FOILAUTH_SCHEME "otpauth"

FoilAuthToken::FoilAuthToken() :
    iDigits(DEFAULT_DIGITS),
    iTimeShift(DEFAULT_TIMESHIFT)
{
}

FoilAuthToken::FoilAuthToken(const FoilAuthToken& aToken) :
    iBytes(aToken.iBytes),
    iLabel(aToken.iLabel),
    iIssuer(aToken.iIssuer),
    iDigits(aToken.iDigits),
    iTimeShift(aToken.iTimeShift)
{
}

FoilAuthToken::FoilAuthToken(QByteArray aBytes, QString aLabel,
    QString aIssuer, int aDigits, int aTimeShift) :
    iBytes(aBytes),
    iLabel(aLabel),
    iIssuer(aIssuer),
    iDigits((aDigits >= 1 && aDigits <= MAX_DIGITS) ? aDigits : DEFAULT_DIGITS),
    iTimeShift(aTimeShift)
{
}

FoilAuthToken& FoilAuthToken::operator=(const FoilAuthToken& aToken)
{
    iBytes = aToken.iBytes;
    iLabel = aToken.iLabel;
    iIssuer = aToken.iIssuer;
    iDigits = aToken.iDigits;
    iTimeShift = aToken.iTimeShift;
    return *this;
}

bool FoilAuthToken::equals(const FoilAuthToken& aToken) const
{
    return iDigits == aToken.iDigits &&
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
    return FoilAuth::TOTP(iBytes, aTime, maxPass);
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
        QByteArray label, secret, issuer, digits;
        while (pos.ptr < pos.end && pos.ptr[0] != '?') {
            label.append(*pos.ptr++);
        }

        FoilBytes secretTag;
        FoilBytes issuerTag;
        FoilBytes digitsTag;
        foil_bytes_from_string(&secretTag, FOILAUTH_KEY_SECRET "=");
        foil_bytes_from_string(&issuerTag, FOILAUTH_KEY_ISSUER "=");
        foil_bytes_from_string(&digitsTag, FOILAUTH_KEY_DIGITS "=");
        QByteArray* value;

        while (pos.ptr < pos.end) {
            pos.ptr++;
            if (foil_parse_skip_bytes(&pos, &secretTag)) {
                value = &secret;
            } else if (foil_parse_skip_bytes(&pos, &issuerTag)) {
                value = &issuer;
            } else if (foil_parse_skip_bytes(&pos, &digitsTag)) {
                value = &digits;
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
                iTimeShift = DEFAULT_TIMESHIFT;

                if (!digits.isEmpty()) {
                    const int n = digits.toInt();
                    if (n >=  MIN_DIGITS && n <= MAX_DIGITS) {
                        iDigits = n;
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
        return buf;
    }
    return QString();
}

QByteArray FoilAuthToken::toQrCode() const
{
    QByteArray out;
    if (isValid()) {
        QString uri = toUri();
        QRcode* code = QRcode_encodeString(qPrintable(uri), 0, QR_ECLEVEL_M, QR_MODE_8, TRUE);
        if (code) {
            const int bytesPerRow = (code->width + 7) / 8;
            if (bytesPerRow > 0) {
                out.reserve(bytesPerRow * code->width);
                for (int y = 0; y < code->width; y++) {
                    const unsigned char* row = code->data + (code->width * y);
                    char c = (row[0] & 1);
                    int x = 1;
                    for (; x < code->width; x++) {
                        if (!(x % 8)) {
                            out.append(&c, 1);
                            c = row[x] & 1;
                        } else {
                            c = (c << 1) | (row[x] & 1);
                        }
                    }
                    const int rem = x % 8;
                    if (rem) {
                        // Most significant bit first
                        c <<= (8 - rem);
                    }
                    out.append(&c, 1);
                }
            }
            QRcode_free(code);
        }
    }
    return out;
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
    }
    return out;
}
