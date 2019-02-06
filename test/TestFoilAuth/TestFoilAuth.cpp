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

#include "FoilAuth.h"
#include "FoilAuthToken.h"

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
 * fromBase32
 *==========================================================================*/

static
void
test_fromBase32(
    void)
{
    static const char out[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
        0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
        0x15, 0x16, 0x17, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };
    QString in1("AEBAGBAFAYDQQCIKBMGA2DQPCAIREEYUCULBOGI2DMOB2HQ7");
    QString in2("aebagbaf aydqqcik bmga2dqp caireeyu culbogi2 dmob2hq7");
    QByteArray out1(FoilAuth::fromBase32(in1));
    QByteArray out2(FoilAuth::fromBase32(in2));
    g_assert(out1 == out2);
    g_assert(out1 == QByteArray(out, sizeof(out)));
    g_assert(FoilAuth::fromBase32("ae") == QByteArray(out, 1));
    g_assert(FoilAuth::fromBase32("aeb").isEmpty());
    g_assert(FoilAuth::fromBase32("aeba") == QByteArray(out, 2));
    g_assert(FoilAuth::fromBase32("aebag") == QByteArray(out, 3));
    g_assert(FoilAuth::fromBase32("aebagb").isEmpty());
    g_assert(FoilAuth::fromBase32("aebagba") == QByteArray(out, 4));
    g_assert(FoilAuth::fromBase32("aebagbaf") == QByteArray(out, 5));
    g_assert(FoilAuth::fromBase32("aebagbafa") == QByteArray(out, 5));
    g_assert(FoilAuth::fromBase32("aebagbafay") == QByteArray(out, 6));
    g_assert(FoilAuth::fromBase32(QString()).isEmpty());
    g_assert(FoilAuth::fromBase32(" ").isEmpty());
    g_assert(FoilAuth::fromBase32("01234567").isEmpty());
    g_assert(FoilAuth::fromBase32("88888888").isEmpty());
    g_assert(FoilAuth::fromBase32("{}").isEmpty());
    g_assert(FoilAuth::fromBase32("[]").isEmpty());
}

/*==========================================================================*
 * base32pad
 *==========================================================================*/

static
void
test_base32pad(
    void)
{
    static const char out[] = {
        0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23
    };
    QString in1("DMOB2HQ7EA=====");   // One pad character missing
    QString in2("DMOB2HQ7EAQQ====="); // One extra pagging character
    QString in3("DMOB2HQ7EAQSE== ="); // Space is ignored
    QString in4("DMOB2HQ7EAQSEIY=");
    g_assert(FoilAuth::fromBase32(in1) == QByteArray(out, sizeof(out) - 3));
    g_assert(FoilAuth::fromBase32(in2) == QByteArray(out, sizeof(out) - 2));
    g_assert(FoilAuth::fromBase32(in3) == QByteArray(out, sizeof(out) - 1));
    g_assert(FoilAuth::fromBase32(in4) == QByteArray(out, sizeof(out)));
    g_assert(FoilAuth::fromBase32(QString("=================")).isEmpty());
    g_assert(FoilAuth::fromBase32(QString("=DMOB2HQ7EAQSEIY=")).isEmpty());
    g_assert(FoilAuth::fromBase32(QString("DMOB2HQ7EB=")).isEmpty());
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
 * rfc4648
 *==========================================================================*/

static
void
test_rfc4648(
    void)
{
    // Test vectors from RFC 4648
    static const char* test[][2] = {
        { "f", "MY======" },
        { "fo", "MZXQ====" },
        { "foo", "MZXW6===" },
        { "foob", "MZXW6YQ=" },
        { "fooba", "MZXW6YTB" },
        { "foobar", "MZXW6YTBOI======" }
    };

    for (guint i = 0; i < G_N_ELEMENTS(test); i++) {
        QByteArray data(test[i][0]);
        QString base32(test[i][1]);
        g_assert(FoilAuth::isValidBase32(base32));
        g_assert(FoilAuth::fromBase32(base32) == data);
        g_assert(FoilAuth::toBase32(data, false) == base32);
    }
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
    QByteArray secret = FoilAuth::fromBase32("VHIIKTVJC6MEOFTJ");
    g_assert(FoilAuth::TOTP(secret, 1548529350, 1000000) == 38068);
}

/*==========================================================================*
 * qrCode
 *==========================================================================*/

static
void
test_qrCode(
    void)
{
    g_assert(FoilAuth::tokenQrCode("", "Label", "Issuer", 5, 0).isEmpty());
    QString code = FoilAuth::tokenQrCode("aebagbafay", "Label", "Issuer", 5, 0);
    g_assert(code == "72axpk7yqjj5jiqixkkgo2xixjq7nyxixjdwe4xiqlia52qi72vk"
        "vk7yaaw32gaaumt7kajifbgkchsyo2lpk4kivgu2flsazyqbrh2i"
        "7tuvxeqinoerowly6w3mfbgqqoff6gsajunawgjyp6lq2mfiewob"
        "sd2yjlkqxeqabugr7xqy636xs7nylsqlklgqc6vpkeyapqrklhyi"
        "2zppluuicap2fmky66hbsh4yad7xtggy734rocvyqifmxogqxjpf"
        "4h4axioexxeaxkkw6reiqiedugca73isqkei");
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
    g_assert(map.count() == 1);
    g_assert(map.contains(FoilAuthToken::KEY_VALID));
    g_assert(!map.value(FoilAuthToken::KEY_VALID).toBool());

    // Valid token
    map = FoilAuth::parseUri("otpauth://totp/Test?secret=vhiiktvjc6meoftj&issuer=Issuer&digits=5");
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

#define TEST_(name) "/FoilAuth/" name

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("basic"), test_basic);
    g_test_add_func(TEST_("toSize"), test_toSize);
    g_test_add_func(TEST_("isValidBase32"), test_isValidBase32);
    g_test_add_func(TEST_("fromBase32"), test_fromBase32);
    g_test_add_func(TEST_("base32pad"), test_base32pad);
    g_test_add_func(TEST_("rfc4648"), test_rfc4648);
    g_test_add_func(TEST_("toBase32"), test_toBase32);
    g_test_add_func(TEST_("fileArray"), test_byteArray);
    g_test_add_func(TEST_("file"), test_file);
    g_test_add_func(TEST_("totp"), test_totp);
    g_test_add_func(TEST_("qrCode"), test_qrCode);
    g_test_add_func(TEST_("parseUri"), test_parseUri);
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C++
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
