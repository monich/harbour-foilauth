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

#include "FoilAuth.h"
#include "FoilAuthToken.h"

#include "HarbourBase32.h"
#include "HarbourDebug.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>

/*==========================================================================*
 * basic
 *==========================================================================*/

static
void
test_basic(
    void)
{
    QObject* obj = FoilAuth::createSingleton(NULL, NULL);
    FoilAuth* foil = qobject_cast<FoilAuth*>(obj);

    g_assert(foil);
    g_assert(!foil->otherFoilAppsInstalled());
    delete foil;
}

/*==========================================================================*
 * toSize
 *==========================================================================*/

static
void
test_toSize(
    void)
{
    g_assert(!FoilAuth::toSize("").isValid());
    g_assert(!FoilAuth::toSize("1").isValid());
    g_assert(!FoilAuth::toSize("1,1").isValid());
    g_assert(!FoilAuth::toSize("1 1").isValid());
    g_assert(!FoilAuth::toSize("1x1x1").isValid());
    g_assert(!FoilAuth::toSize("axb").isValid());
    g_assert(!FoilAuth::toSize("1xb").isValid());
    g_assert(!FoilAuth::toSize("-1x1").isValid());
    g_assert(!FoilAuth::toSize("1x-1").isValid());
    g_assert(!FoilAuth::toSize("0x0").isValid());
    g_assert(FoilAuth::toSize("1x2") == QSize(1,2));
}

/*==========================================================================*
 * isValidBase32
 *==========================================================================*/

static
void
test_isValidBase32(
    void)
{
    g_assert(FoilAuth::isValidBase32("AEBAGBAFAYDQQCIKBMGA2DQPCAIREEYUCULBOGI2DMOB2HQ7"));
    g_assert(FoilAuth::isValidBase32("aebagbaf aydqqcik bmga2dqp caireeyu culbogi2 dmob2hq7"));
    g_assert(FoilAuth::isValidBase32("ae"));
    g_assert(!FoilAuth::isValidBase32("aeb"));
    g_assert(FoilAuth::isValidBase32("aeba"));
    g_assert(FoilAuth::isValidBase32("aebag"));
    g_assert(!FoilAuth::isValidBase32("aebagb"));
    g_assert(FoilAuth::isValidBase32("aebagba"));
    g_assert(FoilAuth::isValidBase32("aebagbaf"));
    g_assert(FoilAuth::isValidBase32("aebagbafa"));
    g_assert(FoilAuth::isValidBase32("aebagbafay"));
    g_assert(!FoilAuth::isValidBase32(QString()));
    g_assert(!FoilAuth::isValidBase32(" "));
    g_assert(!FoilAuth::isValidBase32("01234567"));
    g_assert(!FoilAuth::isValidBase32("88888888"));
    g_assert(!FoilAuth::isValidBase32("{}"));
    g_assert(!FoilAuth::isValidBase32("[]"));
}

/*==========================================================================*
 * toBase32
 *==========================================================================*/

static
void
test_toBase32(
    void)
{
    static const char in[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
        0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
        0x15, 0x16, 0x17, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };
    QString out("aebagbafaydqqcikbmga2dqpcaireeyuculbogi2dmob2hq7");
    g_assert(FoilAuth::toBase32(QByteArray()).isEmpty());
    g_assert(FoilAuth::toBase32(QByteArray(in, sizeof(in))) == out);
}

/*==========================================================================*
 * byteArray
 *==========================================================================*/

static
void
test_byteArray(
    void)
{
    g_assert(FoilAuth::toByteArray(NULL).isEmpty());

    static const uchar data[] = { 0x01, 0x02, 0x03, 0x04 };
    GBytes* bytes = g_bytes_new_static(data, 0);
    g_assert(FoilAuth::toByteArray(bytes).isEmpty());
    g_bytes_unref(bytes);

    bytes = g_bytes_new_static(data, sizeof(data));
    QByteArray array = FoilAuth::toByteArray(bytes);
    g_assert(array.size() == sizeof(data));
    g_assert(!memcmp(array.constData(), data, sizeof(data)));
    g_bytes_unref(bytes);
}

/*==========================================================================*
 * file
 *==========================================================================*/

static
void
test_file(
    void)
{
    QString dir = QCoreApplication::applicationDirPath();
    QString file = FoilAuth::createEmptyFoilFile(dir);
    HDEBUG(file);
    g_assert(!QFileInfo(file).size());
    g_assert(QFile::remove(file));
}

/*==========================================================================*
 * totp
 *==========================================================================*/

static
void
test_totp(
    void)
{
    const QByteArray secret(HarbourBase32::fromBase32("VHIIKTVJC6MEOFTJ"));
    g_assert_cmpuint(FoilAuth::TOTP(secret, 1548529350, 1000000), == ,38068);
}

/*==========================================================================*
 * hotp
 *==========================================================================*/

static
void
test_hotp(
    void)
{
    const QByteArray secret(HarbourBase32::fromBase32("MHGU3YYJJD6W44KUVED4FODUNN4JHJNQ"));
    g_assert_cmpuint(FoilAuth::HOTP(secret, 0, 1000000,
        (FoilAuth::DigestAlgorithm)-1 /* DigestAlgorithmSHA1 */), == ,207601);
    g_assert_cmpuint(FoilAuth::HOTP(secret, 1, 1000000,
        (FoilAuth::DigestAlgorithm)-1 /* DigestAlgorithmSHA1 */), == ,444239);
    g_assert_cmpuint(FoilAuth::HOTP(secret, 0, 1000000,
        FoilAuth::DigestAlgorithmSHA1), == ,207601);
    g_assert_cmpuint(FoilAuth::HOTP(secret, 1, 1000000,
        FoilAuth::DigestAlgorithmSHA1), == ,444239);
    g_assert_cmpuint(FoilAuth::HOTP(secret, 0, 1000000,
        FoilAuth::DigestAlgorithmSHA256), == , 367047);
    g_assert_cmpuint(FoilAuth::HOTP(secret, 1, 1000000,
        FoilAuth::DigestAlgorithmSHA256), == , 714922);
    g_assert_cmpuint(FoilAuth::HOTP(secret, 0, 1000000,
        FoilAuth::DigestAlgorithmSHA512), == , 308534);
    g_assert_cmpuint(FoilAuth::HOTP(secret, 1, 1000000,
        FoilAuth::DigestAlgorithmSHA512), == , 899828);
}

/*==========================================================================*
 * toUri
 *==========================================================================*/

static
void
test_toUri(
    void)
{
    g_assert(FoilAuth::toUri(FoilAuth::TypeTOTP,
        "", "Label", "Issuer", 5,
        FoilAuthTypes::DEFAULT_COUNTER,
        FoilAuthTypes::DEFAULT_TIMESHIFT,
        FoilAuth::DefaultAlgorithm).isEmpty());
    g_assert(FoilAuth::toUri(FoilAuth::TypeTOTP,
        "aebagbafa", "Label", "Issuer", 5,
        FoilAuthTypes::DEFAULT_COUNTER,
        FoilAuthTypes::DEFAULT_TIMESHIFT,
        FoilAuth::DefaultAlgorithm) ==
        "otpauth://totp/Label?secret=aebagbaf&issuer=Issuer&digits=5");
    g_assert(FoilAuth::toUri(FoilAuth::TypeTOTP,
        "aebagbafa", "Label", "Issuer",
        FoilAuthTypes::DEFAULT_DIGITS,
        FoilAuthTypes::DEFAULT_COUNTER,
        FoilAuthTypes::DEFAULT_TIMESHIFT,
        FoilAuth::DefaultAlgorithm) ==
        "otpauth://totp/Label?secret=aebagbaf&issuer=Issuer&digits=6");
    g_assert(FoilAuth::toUri(FoilAuth::TypeTOTP,
        "aebagbafa", "", "",
        FoilAuthTypes::DEFAULT_DIGITS,
        FoilAuthTypes::DEFAULT_COUNTER,
        10,
        FoilAuth::DefaultAlgorithm) ==
        "otpauth://totp/?secret=aebagbaf&digits=6&timeshift=10");
    g_assert(FoilAuth::toUri(FoilAuth::TypeHOTP,
        "aebagbafa", "Label", "",
        FoilAuthTypes::DEFAULT_DIGITS, 42,
        FoilAuthTypes::DEFAULT_TIMESHIFT,
        FoilAuth::DefaultAlgorithm) ==
        "otpauth://hotp/Label?secret=aebagbaf&digits=6&counter=42");
}

/*==========================================================================*
 * migrationUri
 *==========================================================================*/

static
void
test_migrationUri(
    void)
{
    g_assert(FoilAuth::migrationUri(QByteArray()).isEmpty());

    static const uchar data[] = {
        0x0a, 0x37, 0x0a, 0x0a, 0x5f, 0x7a, 0x82, 0xa1,
        0x79, 0x58, 0x3c, 0x32, 0x48, 0x0e, 0x12, 0x18,
        0x57, 0x6f, 0x72, 0x64, 0x50, 0x72, 0x65, 0x73,
        0x73, 0x3a, 0x54, 0x68, 0x69, 0x6e, 0x6b, 0x69,
        0x6e, 0x67, 0x54, 0x65, 0x61, 0x70, 0x6f, 0x74,
        0x1a, 0x09, 0x57, 0x6f, 0x72, 0x64, 0x50, 0x72,
        0x65, 0x73, 0x73, 0x20, 0x01, 0x28, 0x02, 0x30,
        0x02
    };

    const QString uri(FoilAuth::migrationUri(QByteArray((char*)data, (int)sizeof(data))));
    g_assert(uri == QString("otpauth-migration://offline?data=CjcKCl96gqF5WDwySA4SGFdvcmRQcmVzczpUaGlua2luZ1RlYXBvdBoJV29yZFByZXNzIAEoAjAC"));
}

/*==========================================================================*
 * parseUri
 *==========================================================================*/

static
void
test_parseUri(
    void)
{
    // Invalid token
    QVariantMap map = FoilAuth::parseUri(QString());
    g_assert_cmpint(map.count(), == ,1);
    g_assert(map.contains(FoilAuthToken::KEY_VALID));
    g_assert(!map.value(FoilAuthToken::KEY_VALID).toBool());

    // Valid token
    map = FoilAuth::parseUri("otpauth://totp/Test?secret=vhiiktvjc6meoftj&issuer=Issuer&digits=5");
    g_assert(map.count() == 9);
    g_assert(map.value(FoilAuthToken::KEY_VALID).toBool());
    g_assert_cmpint(map.value(FoilAuthToken::KEY_TYPE).toInt(), == ,FoilAuthToken::AuthTypeTOTP);
    g_assert(map.value(FoilAuthToken::KEY_LABEL).toString() == QString("Test"));
    g_assert(map.value(FoilAuthToken::KEY_SECRET).toString() == QString("vhiiktvjc6meoftj"));
    g_assert(map.value(FoilAuthToken::KEY_ISSUER).toString() == QString("Issuer"));
    g_assert_cmpint(map.value(FoilAuthToken::KEY_DIGITS).toInt(), == ,5);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_COUNTER).toInt(), == ,FoilAuthToken::DEFAULT_COUNTER);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_TIMESHIFT).toInt(), == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_ALGORITHM).toInt(), == ,FoilAuthToken::DEFAULT_ALGORITHM);
}

/*==========================================================================*
 * parseMigrationUri
 *==========================================================================*/

static
void
test_parseMigrationUri(
    void)
{
    QVariantList list = FoilAuth::parseMigrationUri(QString());
    g_assert_cmpint(list.count(), == ,0);

    // One TOTP tocken
    list = FoilAuth::parseMigrationUri(QString("otpauth-migration://offline?data=CjcKCl96gqF5WDwySA4SGFdvcmRQcmVzczpUaGlua2luZ1RlYXBvdBoJV29yZFByZXNzIAEoAjAC"));
    g_assert_cmpint(list.count(), == ,1);

    QVariantMap map = list.at(0).toMap();
    g_assert(map.count() == 9);

    QByteArray label, secret, issuer;
    g_assert(map.value(FoilAuthToken::KEY_VALID).toBool());
    g_assert_cmpint(map.value(FoilAuthToken::KEY_TYPE).toInt(), == ,FoilAuthToken::AuthTypeTOTP);
    label = map.value(FoilAuthToken::KEY_LABEL).toString().toUtf8();
    secret = map.value(FoilAuthToken::KEY_SECRET).toString().toUtf8();
    issuer = map.value(FoilAuthToken::KEY_ISSUER).toString().toUtf8();
    g_assert_cmpstr(label, == ,"WordPress:ThinkingTeapot");
    g_assert_cmpstr(secret, == ,"l55ifilzla6desao");
    g_assert_cmpstr(issuer, == ,"WordPress");
    g_assert_cmpint(map.value(FoilAuthToken::KEY_DIGITS).toInt(), == ,8);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_COUNTER).toInt(), == ,FoilAuthToken::DEFAULT_COUNTER);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_TIMESHIFT).toInt(), == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_ALGORITHM).toInt(), == ,FoilAuthToken::DEFAULT_ALGORITHM);

    // One HOTP + one TOTP token
    list = FoilAuth::parseMigrationUri(QString("otpauth-migration://offline?data=CikKFGHNTeMJSP1ucVSpB8K4dGt4k6WwEglIT1RQIHRlc3QgASgBMAE4AwojChDGzIcWUVA5HichlIduvZrUEglUT1RQIHRlc3QgASgBMAIQARgBIAAo%2BOCI%2Bvn%2F%2F%2F%2F%2FAQ%3D%3D"));
    g_assert_cmpint(list.count(), == ,2);

    // HOTP
    map = list.at(0).toMap();
    g_assert(map.count() == 9);
    g_assert(map.value(FoilAuthToken::KEY_VALID).toBool());
    g_assert_cmpint(map.value(FoilAuthToken::KEY_TYPE).toInt(), == ,FoilAuthToken::AuthTypeHOTP);
    label = map.value(FoilAuthToken::KEY_LABEL).toString().toUtf8();
    secret = map.value(FoilAuthToken::KEY_SECRET).toString().toUtf8();
    g_assert(map.value(FoilAuthToken::KEY_ISSUER).toString().isEmpty());
    g_assert_cmpstr(label, == ,"HOTP test");
    g_assert_cmpstr(secret, == ,"mhgu3yyjjd6w44kuved4fodunn4jhjnq");
    g_assert_cmpint(map.value(FoilAuthToken::KEY_DIGITS).toInt(), == ,FoilAuthTypes::DEFAULT_DIGITS);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_COUNTER).toInt(), == ,3);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_TIMESHIFT).toInt(), == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_ALGORITHM).toInt(), == ,FoilAuthToken::DEFAULT_ALGORITHM);

    // TOTP
    map = list.at(1).toMap();
    g_assert(map.count() == 9);
    g_assert(map.value(FoilAuthToken::KEY_VALID).toBool());
    g_assert_cmpint(map.value(FoilAuthToken::KEY_TYPE).toInt(), == ,FoilAuthToken::AuthTypeTOTP);
    label = map.value(FoilAuthToken::KEY_LABEL).toString().toUtf8();
    secret = map.value(FoilAuthToken::KEY_SECRET).toString().toUtf8();
    g_assert(map.value(FoilAuthToken::KEY_ISSUER).toString().isEmpty());
    g_assert_cmpstr(label, == ,"TOTP test");
    g_assert_cmpstr(secret, == ,"y3giofsrka4r4jzbssdw5pm22q======");
    g_assert_cmpint(map.value(FoilAuthToken::KEY_DIGITS).toInt(), == ,FoilAuthTypes::DEFAULT_DIGITS);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_COUNTER).toInt(), == ,FoilAuthToken::DEFAULT_COUNTER);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_TIMESHIFT).toInt(), == ,FoilAuthToken::DEFAULT_TIMESHIFT);
    g_assert_cmpint(map.value(FoilAuthToken::KEY_ALGORITHM).toInt(), == ,FoilAuthToken::DEFAULT_ALGORITHM);
}

/*==========================================================================*
 * stringListRemove
 *==========================================================================*/

static
void
test_stringListRemove(
    void)
{
    QStringList list;

    list.append("foo");
    list.append("bar");
    g_assert(FoilAuth::stringListRemove(list, "foobar") == list);
    g_assert(FoilAuth::stringListRemove(list, "foo") != list);
}

/*==========================================================================*
 * Common
 *==========================================================================*/

#define TEST_(name) "/FoilAuth/" name

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("basic"), test_basic);
    g_test_add_func(TEST_("toSize"), test_toSize);
    g_test_add_func(TEST_("isValidBase32"), test_isValidBase32);
    g_test_add_func(TEST_("toBase32"), test_toBase32);
    g_test_add_func(TEST_("fileArray"), test_byteArray);
    g_test_add_func(TEST_("file"), test_file);
    g_test_add_func(TEST_("totp"), test_totp);
    g_test_add_func(TEST_("hotp"), test_hotp);
    g_test_add_func(TEST_("toUri"), test_toUri);
    g_test_add_func(TEST_("migrationUri"), test_migrationUri);
    g_test_add_func(TEST_("parseUri"), test_parseUri);
    g_test_add_func(TEST_("parseMigrationUri"), test_parseMigrationUri);
    g_test_add_func(TEST_("stringListRemove"), test_stringListRemove);
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C++
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
