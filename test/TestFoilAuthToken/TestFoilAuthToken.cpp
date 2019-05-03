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
    g_assert(token.iDigits == FoilAuthToken::DEFAULT_DIGITS);
    g_assert(token.iTimeShift == FoilAuthToken::DEFAULT_TIMESHIFT);

    g_assert(token.parseUri("otpauth://totp/?secret=VHIIKTVJ&digits=2"));
    g_assert(token.iBytes == QByteArray((char*)bytes, sizeof(bytes)/2));
    g_assert(token.iLabel.isEmpty());
    g_assert(token.iIssuer.isEmpty());
    g_assert(token.iDigits == 2);
    g_assert(token.iTimeShift == FoilAuthToken::DEFAULT_TIMESHIFT);

    g_assert(token.parseUri("otpauth://totp/?secret=VHIIKTVJ&digits=23"));
    g_assert(token.iBytes == QByteArray((char*)bytes, sizeof(bytes)/2));
    g_assert(token.iLabel.isEmpty());
    g_assert(token.iIssuer.isEmpty());
    g_assert(token.iDigits == FoilAuthToken::DEFAULT_DIGITS);
    g_assert(token.iTimeShift == FoilAuthToken::DEFAULT_TIMESHIFT);

    g_assert(token.parseUri("otpauth://totp/?secret=VHIIKTVJ&digits=x"));
    g_assert(token.iBytes == QByteArray((char*)bytes, sizeof(bytes)/2));
    g_assert(token.iLabel.isEmpty());
    g_assert(token.iIssuer.isEmpty());
    g_assert(token.iDigits == FoilAuthToken::DEFAULT_DIGITS);
    g_assert(token.iTimeShift == FoilAuthToken::DEFAULT_TIMESHIFT);
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
    g_assert(map.count() == 7);
    g_assert(map.value(FoilAuthToken::KEY_VALID).toBool());
    g_assert(map.value(FoilAuthToken::KEY_TYPE).toString() == FoilAuthToken::TYPE_TOTP);
    g_assert(map.value(FoilAuthToken::KEY_LABEL).toString() == QString("Test"));
    g_assert(map.value(FoilAuthToken::KEY_SECRET).toString() == QString("vhiiktvjc6meoftj"));
    g_assert(map.value(FoilAuthToken::KEY_ISSUER).toString() == QString("Issuer"));
    g_assert(map.value(FoilAuthToken::KEY_DIGITS).toInt() == 5);
    g_assert(map.value(FoilAuthToken::KEY_TIMESHIFT).toInt() == 0);
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
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C++
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
