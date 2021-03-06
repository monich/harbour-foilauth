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
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
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

#include <QCoreApplication>

#include <glib.h>

/*==========================================================================*
 * basic
 *==========================================================================*/

static
void
test_basic(
    void)
{
    QByteArray data("secret");
    FoilAuthToken token1(data, "Label", "Issuer", 1, 10);
    FoilAuthToken token2(token1);
    FoilAuthToken token3;

    g_assert(token1 == token2);
    g_assert(token1 != token3);
    token3 = token2;
    g_assert(token1 == token3);

    g_assert(!token3.setDigits(0));
    g_assert(token1 == token3);

    g_assert(!token3.setDigits(FoilAuthToken::MAX_DIGITS + 1));
    g_assert(token1 == token3);

    g_assert(!token3.setAlgorithm((FoilAuthToken::DigestAlgorithm)-1));
    g_assert(token1 == token3);

    g_assert(token3.setDigits(token3.iDigits + 1));
    g_assert(token1 != token3);
}

/*==========================================================================*
 * password
 *==========================================================================*/

static
void
test_password(
    void)
{
    QByteArray data = FoilAuth::fromBase32("VHIIKTVJC6MEOFTJ");
    FoilAuthToken token(data, "Label", "Issuer");
    g_assert(token.passwordString(1548529350) == QString("038068"));
}

/*==========================================================================*
 * parseUri
 *==========================================================================*/

static
void
test_parseUri(
    void)
{
    static const uchar bytes[] = {
        0xA9, 0xD0, 0x85, 0x4E, 0xA9, 0x17, 0x98, 0x47, 0x16, 0x69
    };

    FoilAuthToken token;
    g_assert(!token.parseUri("otpauth://totp/"));
    g_assert(!token.parseUri("otpauth://totp/....."));
    g_assert(!token.parseUri("otpauth://totp/Test Secret?issuer=Test Issuer"));
    g_assert(!token.parseUri("auth://totp/Test Secret?issuer=Test Issuer"));

    g_assert(token.parseUri("otpauth://totp/Test Secret?secret=VHIIKTVJC6MEOFTJ&issuer=Test Issuer&foo=bar"));
    g_assert(token.iBytes == QByteArray((char*)bytes, sizeof(bytes)));
    g_assert(token.iLabel == QString("Test Secret"));
    g_assert(token.iIssuer == QString("Test Issuer"));
    g_assert_cmpint(token.iDigits, == ,FoilAuthToken::DEFAULT_DIGITS);
    g_assert_cmpint(token.iTimeShift, == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(token.iAlgorithm, == ,FoilAuthToken::DEFAULT_ALGORITHM);

    g_assert(token.parseUri("otpauth://totp/?secret=VHIIKTVJ&digits=2&algorithm=foo"));
    g_assert(token.iBytes == QByteArray((char*)bytes, sizeof(bytes)/2));
    g_assert(token.iLabel.isEmpty());
    g_assert(token.iIssuer.isEmpty());
    g_assert_cmpint(token.iDigits, == ,2);
    g_assert_cmpint(token.iTimeShift, == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(token.iAlgorithm, == ,FoilAuthToken::DEFAULT_ALGORITHM);

    g_assert(token.parseUri("otpauth://totp/?secret=VHIIKTVJ&digits=23&algorithm=MD5"));
    g_assert(token.iBytes == QByteArray((char*)bytes, sizeof(bytes)/2));
    g_assert(token.iLabel.isEmpty());
    g_assert(token.iIssuer.isEmpty());
    g_assert_cmpint(token.iDigits, == ,FoilAuthToken::DEFAULT_DIGITS);
    g_assert_cmpint(token.iTimeShift, == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(token.iAlgorithm, == ,FoilAuthToken::DigestAlgorithmMD5);

    g_assert(token.parseUri("otpauth://totp/?secret=VHIIKTVJ&digits=x&algorithm=SHA1"));
    g_assert(token.iBytes == QByteArray((char*)bytes, sizeof(bytes)/2));
    g_assert(token.iLabel.isEmpty());
    g_assert(token.iIssuer.isEmpty());
    g_assert_cmpint(token.iDigits, == ,FoilAuthToken::DEFAULT_DIGITS);
    g_assert_cmpint(token.iTimeShift, == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(token.iAlgorithm, == ,FoilAuthToken::DigestAlgorithmSHA1);

    g_assert(token.parseUri("otpauth://totp/?secret=VHIIKTVJ&digits=x&algorithm=SHA256"));
    g_assert(token.iBytes == QByteArray((char*)bytes, sizeof(bytes)/2));
    g_assert(token.iLabel.isEmpty());
    g_assert(token.iIssuer.isEmpty());
    g_assert_cmpint(token.iDigits, == ,FoilAuthToken::DEFAULT_DIGITS);
    g_assert_cmpint(token.iTimeShift, == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(token.iAlgorithm, == ,FoilAuthToken::DigestAlgorithmSHA256);

    g_assert(token.parseUri("otpauth://totp/?secret=VHIIKTVJ&digits=x&algorithm=SHA512"));
    g_assert(token.iBytes == QByteArray((char*)bytes, sizeof(bytes)/2));
    g_assert(token.iLabel.isEmpty());
    g_assert(token.iIssuer.isEmpty());
    g_assert_cmpint(token.iDigits, == ,FoilAuthToken::DEFAULT_DIGITS);
    g_assert_cmpint(token.iTimeShift, == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(token.iAlgorithm, == ,FoilAuthToken::DigestAlgorithmSHA512);
}

/*==========================================================================*
 * toUri
 *==========================================================================*/

static
void
test_toUri(
    void)
{
    static const uchar bytes[] = {
        0xA9, 0xD0, 0x85, 0x4E, 0xA9, 0x17, 0x98, 0x47, 0x16, 0x69
    };

    FoilAuthToken token;
    g_assert(token.toUri().isEmpty());

    token = FoilAuthToken(QByteArray((char*)bytes, sizeof(bytes)),
        QString(), QString());
    g_assert(token.toUri() == QString("otpauth://totp/?secret=vhiiktvjc6meoftj&digits=6"));

    token = FoilAuthToken(QByteArray((char*)bytes, sizeof(bytes)),
        "Test Secret", "Test Issuer", 5);

    QString uri(token.toUri());
    g_assert(uri == QString("otpauth://totp/Test Secret?secret=vhiiktvjc6meoftj&issuer=Test Issuer&digits=5"));

    FoilAuthToken token2;
    g_assert(token2.parseUri(uri));
    g_assert(token2 == token);

    // Algorithm
    uri = FoilAuthToken(QByteArray((char*)bytes, sizeof(bytes)), "Test",
        QString(), 6, FoilAuthToken::DEFAULT_TIMESHIFT,
        FoilAuthToken::DigestAlgorithmMD5).toUri();
    g_assert(uri == QString("otpauth://totp/Test?secret=vhiiktvjc6meoftj&digits=6&algorithm=MD5"));

    uri = FoilAuthToken(QByteArray((char*)bytes, sizeof(bytes)), "Test",
        QString(), 6, FoilAuthToken::DEFAULT_TIMESHIFT,
        FoilAuthToken::DigestAlgorithmSHA256).toUri();
    g_assert(uri == QString("otpauth://totp/Test?secret=vhiiktvjc6meoftj&digits=6&algorithm=SHA256"));

    uri = FoilAuthToken(QByteArray((char*)bytes, sizeof(bytes)), "Test",
        QString(), 6, FoilAuthToken::DEFAULT_TIMESHIFT,
        FoilAuthToken::DigestAlgorithmSHA512).toUri();
    g_assert(uri == QString("otpauth://totp/Test?secret=vhiiktvjc6meoftj&digits=6&algorithm=SHA512"));
}

/*==========================================================================*
 * toVariantMap
 *==========================================================================*/

static
void
test_toVariantMap(
    void)
{
    FoilAuthToken token;

    // Invalid token
    QVariantMap map = token.toVariantMap();
    g_assert(map.count() == 1);
    g_assert(map.contains(FoilAuthToken::KEY_VALID));
    g_assert(!map.value(FoilAuthToken::KEY_VALID).toBool());

    // Valid token
    g_assert(token.parseUri("otpauth://totp/Test?secret=vhiiktvjc6meoftj&issuer=Issuer&digits=5"));
    map = token.toVariantMap();
    g_assert_cmpint(map.count(), == ,8);
    g_assert(map.value(FoilAuthToken::KEY_VALID).toBool());
    g_assert(map.value(FoilAuthToken::KEY_TYPE).toString() == FoilAuthToken::TYPE_TOTP);
    g_assert(map.value(FoilAuthToken::KEY_LABEL).toString() == QString("Test"));
    g_assert(map.value(FoilAuthToken::KEY_SECRET).toString() == QString("vhiiktvjc6meoftj"));
    g_assert(map.value(FoilAuthToken::KEY_ISSUER).toString() == QString("Issuer"));
    g_assert_cmpint(map.value(FoilAuthToken::KEY_DIGITS).toInt(), == ,5);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_TIMESHIFT).toInt(), == ,0);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_ALGORITHM).toInt(), == ,FoilAuthToken::DEFAULT_ALGORITHM);
}

/*==========================================================================*
 * test_parseProtoBuf
 *==========================================================================*/

static
void
test_parseProtoBuf(
    void)
{
    static const uchar bytes[] = {
        0x0a, 0x51, 0x0a, 0x14, 0x85, 0x29, 0xe2, 0x03,
        0x46, 0x3b, 0xa2, 0x66, 0x0f, 0x5a, 0x93, 0x75,
        0x85, 0x53, 0xd5, 0x52, 0x24, 0x25, 0x60, 0x06,
        0x12, 0x20, 0x45, 0x78, 0x61, 0x6d, 0x70, 0x6c,
        0x65, 0x20, 0x43, 0x6f, 0x6d, 0x70, 0x61, 0x6e,
        0x79, 0x3a, 0x74, 0x65, 0x73, 0x74, 0x40, 0x65,
        0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63,
        0x6f, 0x6d, 0x1a, 0x0f, 0x45, 0x78, 0x61, 0x6d,
        0x70, 0x6c, 0x65, 0x20, 0x43, 0x6f, 0x6d, 0x70,
        0x61, 0x6e, 0x79, 0x20, 0x01, 0x28, 0x01, 0x30,
        0x02, 0x38, 0x00,

        0x0a, 0x37, 0x0a, 0x0a, 0x5f, 0x7a, 0x82, 0xa1,
        0x79, 0x58, 0x3c, 0x32, 0x48, 0x0e, 0x12, 0x18,
        0x57, 0x6f, 0x72, 0x64, 0x50, 0x72, 0x65, 0x73,
        0x73, 0x3a, 0x54, 0x68, 0x69, 0x6e, 0x6b, 0x69,
        0x6e, 0x67, 0x54, 0x65, 0x61, 0x70, 0x6f, 0x74,
        0x1a, 0x09, 0x57, 0x6f, 0x72, 0x64, 0x50, 0x72,
        0x65, 0x73, 0x73, 0x20, 0x02, 0x28, 0x02, 0x30,
        0x02,

        0x0a, 0x3e, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
        0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
        0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
        0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
        0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
        0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
        0x6f, 0x6d, 0x1a, 0x06, 0x47, 0x6f, 0x6f, 0x67,
        0x6c, 0x65, 0x20, 0x03, 0x28, 0x01, 0x30, 0x02,

        0x0a, 0x3e, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
        0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
        0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
        0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
        0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
        0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
        0x6f, 0x6d, 0x1a, 0x06, 0x47, 0x6f, 0x6f, 0x67,
        0x6c, 0x65, 0x20, 0x04, 0x28, 0x01, 0x30, 0x02,

        0x10, 0x01, 0x18, 0x01, 0x20,
        0x00, 0x28, 0xe7, 0xac, 0xc0, 0xc6, 0xf9, 0xff,
        0xff, 0xff, 0xff, 0x01
    };

    static const uchar secret0[] = {
        0x85, 0x29, 0xe2, 0x03, 0x46, 0x3b, 0xa2, 0x66,
        0x0f, 0x5a, 0x93, 0x75, 0x85, 0x53, 0xd5, 0x52,
        0x24, 0x25, 0x60, 0x06
    };

    static const uchar secret1[] = {
        0x5f, 0x7a, 0x82, 0xa1, 0x79, 0x58, 0x3c, 0x32,
        0x48, 0x0e
    };

    static const uchar secret2[] = {
        0x82, 0x6b, 0xb2, 0x37, 0xa9, 0x4a, 0x4e, 0x68,
        0x71, 0xea, 0xdb, 0x3c, 0x4e, 0xff, 0xd8, 0x65,
        0xa8, 0x2e, 0x8f, 0x97
    };

    QByteArray buf((char*)bytes, sizeof(bytes));
    QList<FoilAuthToken> result(FoilAuthToken::parseProtoBuf(buf));
    g_assert_cmpint(result.count(), == ,4);

    FoilAuthToken token = result.at(0);
    g_assert(token.iBytes == QByteArray((char*)secret0, sizeof(secret0)));
    g_assert(token.iLabel == QString("Example Company:test@example.com"));
    g_assert(token.iIssuer == QString("Example Company"));
    g_assert_cmpint(token.iDigits, == ,6);
    g_assert_cmpint(token.iTimeShift, == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(token.iAlgorithm, == ,FoilAuthToken::DigestAlgorithmSHA1);

    token = result.at(1);
    g_assert(token.iBytes == QByteArray((char*)secret1, sizeof(secret1)));
    g_assert(token.iLabel == QString("WordPress:ThinkingTeapot"));
    g_assert(token.iIssuer == QString("WordPress"));
    g_assert_cmpint(token.iDigits, == ,8);
    g_assert_cmpint(token.iAlgorithm, == ,FoilAuthToken::DigestAlgorithmSHA256);

    token = result.at(2);
    g_assert(token.iBytes == QByteArray((char*)secret2, sizeof(secret2)));
    g_assert(token.iLabel == QString("Google:example@gmail.com"));
    g_assert(token.iIssuer == QString("Google"));
    g_assert_cmpint(token.iDigits, == ,6);
    g_assert_cmpint(token.iAlgorithm, == ,FoilAuthToken::DigestAlgorithmSHA512);

    token = result.at(3);
    g_assert(token.iBytes == QByteArray((char*)secret2, sizeof(secret2)));
    g_assert(token.iLabel == QString("Google:example@gmail.com"));
    g_assert(token.iIssuer == QString("Google"));
    g_assert_cmpint(token.iDigits, == ,6);
    g_assert_cmpint(token.iAlgorithm, == ,FoilAuthToken::DigestAlgorithmMD5);
}

/*==========================================================================*
 * test_parseProtoBufFail
 *==========================================================================*/

#define ARRAY_AND_SIZE(a) a, sizeof(a)

static const guint8 fail_data1[] = {
    0x10, 0x01, 0x18, 0x01, 0x20, 0x00, 0x28, 0xe7,
    0xac, 0xc0, 0xc6, 0xf9, 0xff, 0xff, 0xff, 0xff
    /* Truncated message */
};
static const guint8 fail_data2[] = {
    0x28, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff
    /* Broken varint */
};
static const guint8 fail_data3[] = {
    0x15, 0x01, 0x02, 0x03, 0x04
    /* Unsupported tag type */
};
static const guint8 fail_data4[] = {
    0x10, 0x01, 0x18, 0x01, 0x20, 0x00, 0x28, 0xe7,
    0xac, 0xc0, 0xc6, 0xf9, 0xff, 0xff, 0xff, 0xff,
    0x01, 0x0a /* OtpParameters is completely missing */
};
static const guint8 fail_data5[] = {
    0x0a, 0x3e, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
    0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
    0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
    0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
    0x6f, 0x6d, 0x1a, 0x06, 0x47, 0x6f, 0x6f, 0x67,
    0x6c, 0x65, 0x20, 0x01, 0x28, 0x01, 0x30, 0x03
                           /* Invalid OtpType ^^^^ */
};
static const guint8 fail_data6[] = {
    0x10, 0x01, 0x18, 0x01, 0x20, 0x00, 0x28, 0xe7,
    0xac, 0xc0, 0xc6, 0xf9, 0xff, 0xff, 0xff, 0xff,
    0x01, 0x0a, 0x4e, 0x0a, 0x14, 0x85, 0x29, 0xe2,
    0x03, 0x46, 0x3b, 0xa2, 0x66, 0x0f, 0x5a, 0x93,
    0x75, 0x85, 0x53, 0xd5, 0x52, 0x24, 0x25, 0x60,
    0x06, 0x12, 0x20, 0x45, 0x78, 0x61, 0x6d, 0x70,
    0x6c, 0x65, 0x20, 0x43, 0x6f, 0x6d, 0x70, 0x61,
    0x6e, 0x79, 0x3a, 0x74, 0x65, 0x73, 0x74, 0x40,
    0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e,
    0x63, 0x6f, 0x6d, 0x1a, 0x0f, 0x45, 0x78, 0x61,
    0x6d, 0x70, 0x6c, 0x65, 0x20, 0x43, 0x6f, 0x6d,
    0x70, 0x61, 0x6e, 0x79, 0x20, 0x01, 0x28, 0x01,
    0x30 /* OtpType value is missing */
};
static const guint8 fail_data7[] = {
    0x0a, 0x3e, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
    0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
    0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
    0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
    0x6f, 0x6d, 0x1a, 0x06, 0x47, 0x6f, 0x6f, 0x67,
    0x6c, 0x65, 0x20, 0x07, 0x28, 0x01, 0x30, 0x02
    /* Bad Algorithm  ^^^^ */
};
static const guint8 fail_data8[] = {
    0x0a, 0x39, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
    0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
    0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
    0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
    0x6f, 0x6d, 0x1a, 0x06, 0x47, 0x6f, 0x6f, 0x67,
    0x6c, 0x65, 0x20  /* Algorithm value is missing */
};
static const guint8 fail_data9[] = {
    0x0a, 0x3e, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
    0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
    0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
    0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
    0x6f, 0x6d, 0x1a, 0x06, 0x47, 0x6f, 0x6f, 0x67,
    0x6c, 0x65, 0x20, 0x01, 0x28, 0x07, 0x30, 0x02,
                   /* Bad Digits  ^^^^ */
};
static const guint8 fail_data10[] = {
    0x0a, 0x3b, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
    0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
    0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
    0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
    0x6f, 0x6d, 0x1a, 0x06, 0x47, 0x6f, 0x6f, 0x67,
    0x6c, 0x65, 0x20, 0x01, 0x28 /* Digits value is missing */
};
static const guint8 fail_data11[] = {
    0x0a, 0x3f, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
    0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
    0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
    0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
    0x6f, 0x6d, 0x1a, 0x06, 0x47, 0x6f, 0x6f, 0x67,
    0x6c, 0x65, 0x20, 0x01, 0x28, 0x07, 0x30, 0x01,
    0x38 /* Counter value missing */
};
static const guint8 fail_data12[] = {
    0x0a, 0x3f, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
    0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
    0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
    0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
    0x6f, 0x6d, 0x1a, 0x06, 0x47, 0x6f, 0x6f, 0x67,
    0x6c, 0x65, 0x20, 0x01, 0x28, 0x07, 0x30, 0x01,
    0x78 /* Unsupported tag in OtpParameters block */
};
static const guint8 fail_data13[] = {
    0x0a, 0x32, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
    0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
    0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
    0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
    0x6f, 0x6d, 0x1a, 0x06 /* Issuer value is truncated */
};
static const guint8 fail_data14[] = {
    0x0a, 0x18, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
    0x12, 0x18  /* Name value is missing */
};
static const guint8 fail_data15[] = {
    0x0a, 0x12, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65 /* Secret value is truncated */
};
static const guint8 fail_data16[] = {
    0x0a, 0x34, 0x12, 0x0d, 0x61, 0x75, 0x74, 0x68,
    0x40, 0x74, 0x65, 0x73, 0x74, 0x2e, 0x63, 0x6f,
    0x6d, 0x1a, 0x1d, 0x4d, 0x79, 0x43, 0x6f, 0x6d,
    0x70, 0x61, 0x6e, 0x79, 0x4e, 0x61, 0x6d, 0x65,
    0x2e, 0x41, 0x62, 0x70, 0x5a, 0x65, 0x72, 0x6f,
    0x54, 0x65, 0x6d, 0x70, 0x6c, 0x61, 0x74, 0x65,
    0x20, 0x01, 0x28, 0x01, 0x30, 0x02 /* No Secret */
};
static const guint8 fail_data17[] = {
    0x0a, 0x3f, 0x0a, 0x14, 0x82, 0x6b, 0xb2, 0x37,
    0xa9, 0x4a, 0x4e, 0x68, 0x71, 0xea, 0xdb, 0x3c,
    0x4e, 0xff, 0xd8, 0x65, 0xa8, 0x2e, 0x8f, 0x97,
    0x12, 0x18, 0x47, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
    0x3a, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
    0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63,
    0x6f, 0x6d, 0x1a, 0x06, 0x47, 0x6f, 0x6f, 0x67,
    0x6c, 0x65, 0x20, 0x01, 0x28, 0x07, 0x30, 0x01,
    0xff /* Garbage at the end of OtpParameters block */
};

static const FoilBytes parseProtoBufFail_tests[] = {
    { ARRAY_AND_SIZE(fail_data1) },
    { ARRAY_AND_SIZE(fail_data2) },
    { ARRAY_AND_SIZE(fail_data3) },
    { ARRAY_AND_SIZE(fail_data4) },
    { ARRAY_AND_SIZE(fail_data5) },
    { ARRAY_AND_SIZE(fail_data6) },
    { ARRAY_AND_SIZE(fail_data7) },
    { ARRAY_AND_SIZE(fail_data8) },
    { ARRAY_AND_SIZE(fail_data9) },
    { ARRAY_AND_SIZE(fail_data10) },
    { ARRAY_AND_SIZE(fail_data11) },
    { ARRAY_AND_SIZE(fail_data12) },
    { ARRAY_AND_SIZE(fail_data13) },
    { ARRAY_AND_SIZE(fail_data14) },
    { ARRAY_AND_SIZE(fail_data15) },
    { ARRAY_AND_SIZE(fail_data16) },
    { ARRAY_AND_SIZE(fail_data17) }
};

static
void
test_parseProtoBufFail(
    gconstpointer test_data)
{
    const FoilBytes* bytes = (const FoilBytes*) test_data;
    QByteArray buf((char*)bytes->val, bytes->len);
    QList<FoilAuthToken> result(FoilAuthToken::parseProtoBuf(buf));
    g_assert_cmpint(result.count(), == ,0);
}

/*==========================================================================*
 * Common
 *==========================================================================*/

#define TEST_(name) "/FoilAuthToken/" name

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("basic"), test_basic);
    g_test_add_func(TEST_("password"), test_password);
    g_test_add_func(TEST_("parseUri"), test_parseUri);
    g_test_add_func(TEST_("toUri"), test_toUri);
    g_test_add_func(TEST_("toVariantMap"), test_toVariantMap);
    g_test_add_func(TEST_("parseProtoBuf"), test_parseProtoBuf);
    for (uint i = 0; i < G_N_ELEMENTS(parseProtoBufFail_tests); i++) {
        char* path = g_strdup_printf(TEST_("parseProtoBufFail/%u"), i+1);
        g_test_add_data_func(path, parseProtoBufFail_tests + i,
            test_parseProtoBufFail);
        g_free(path);
    }
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C++
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
