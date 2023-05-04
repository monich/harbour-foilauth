/*
 * Copyright (C) 2019-2023 Slava Monich <slava@monich.com>
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
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer
 *     in the documentation and/or other materials provided with the
 *     distribution.
 *  3. Neither the names of the copyright holders nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING
 * IN ANY WAY OUT OF THE USE OR INABILITY TO USE THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#include "FoilAuth.h"
#include "FoilAuthToken.h"

#include "HarbourBase32.h"
#include "HarbourDebug.h"

#include "foil_hmac.h"
#include "foil_digest.h"
#include "foil_random.h"
#include "foil_output.h"
#include "foil_input.h"
#include "foil_util.h"

#include "gutil_misc.h"

#include <QFile>
#include <QFileSystemWatcher>

#define FOILAPPS_DIR    "/usr/bin"
#define FOILPICS_PATH   FOILAPPS_DIR "/harbour-foilpics"
#define FOILNOTES_PATH  FOILAPPS_DIR "/harbour-foilnotes"

#define FOILAUTH_MIGRATION_PREFIX   "otpauth-migration://offline?data="

// ==========================================================================
// FoilAuth::Private
// ==========================================================================

class FoilAuth::Private :
    public QObject
{
    Q_OBJECT

public:
    Private(FoilAuth*);

    static bool foilPicsInstalled();
    static bool foilNotesInstalled();
    static bool otherFoilAppsInstalled();
    FoilAuth* foilAuth() const;

public Q_SLOTS:
    void checkFoilAppsInstalled();

public:
    QFileSystemWatcher* iFileWatcher;
    bool iOtherFoilAppsInstalled;
};

FoilAuth::Private::Private(FoilAuth* aParent) :
    QObject(aParent),
    iFileWatcher(new QFileSystemWatcher(this))
{
    connect(iFileWatcher, SIGNAL(directoryChanged(QString)),
        SLOT(checkFoilAppsInstalled()));
    if (!iFileWatcher->addPath(FOILAPPS_DIR)) {
        HWARN("Failed to watch " FOILAPPS_DIR);
    }
    iOtherFoilAppsInstalled = otherFoilAppsInstalled();
}

inline
FoilAuth*
FoilAuth::Private::foilAuth() const
{
    return qobject_cast<FoilAuth*>(parent());
}

bool
FoilAuth::Private::foilPicsInstalled()
{
    const bool installed = QFile::exists(FOILPICS_PATH);
    HDEBUG("FoilPics is" << (installed ? "installed" : "not installed"));
    return installed;
}

bool
FoilAuth::Private::foilNotesInstalled()
{
    const bool installed = QFile::exists(FOILNOTES_PATH);
    HDEBUG("FoilNotes is" << (installed ? "installed" : "not installed"));
    return installed;
}

bool
FoilAuth::Private::otherFoilAppsInstalled()
{
    return foilPicsInstalled() || foilNotesInstalled();
}

void
FoilAuth::Private::checkFoilAppsInstalled()
{
    const bool haveOtherFoilApps = otherFoilAppsInstalled();
    if (iOtherFoilAppsInstalled != haveOtherFoilApps) {
        iOtherFoilAppsInstalled = haveOtherFoilApps;
        Q_EMIT foilAuth()->otherFoilAppsInstalledChanged();
    }
}

// ==========================================================================
// FoilAuth
// ==========================================================================

FoilAuth::FoilAuth(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

// Callback for qmlRegisterSingletonType<FoilAuth>
QObject*
FoilAuth::createSingleton(
    QQmlEngine*,
    QJSEngine*)
{
    return new FoilAuth;
}

bool
FoilAuth::otherFoilAppsInstalled() const
{
    return iPrivate->iOtherFoilAppsInstalled;
}

QSize
FoilAuth::toSize(
    QVariant aVariant)
{
    // e.g. "1920x1080"
    if (aVariant.isValid()) {
        QStringList values(aVariant.toString().split('x'));
        if (values.count() == 2) {
            bool ok = false;
            int width = values.at(0).toInt(&ok);
            if (ok && width > 0) {
                int height = values.at(1).toInt(&ok);
                if (ok && height > 0) {
                    return QSize(width, height);
                }
            }
        }
    }
    return QSize();
}

bool
FoilAuth::isValidBase32(
    const QString aBase32)
{
    return HarbourBase32::isValidBase32(aBase32);
}

QString
FoilAuth::toBase32(
    const QByteArray aBinary,
    bool aLowerCase)
{
    return HarbourBase32::toBase32(aBinary, aLowerCase);
}

QByteArray
FoilAuth::toByteArray(
    GBytes* aData)
{
    if (aData) {
        gsize size;
        const char* data = (char*)g_bytes_get_data(aData, &size);
        if (size) {
            return QByteArray(data, size);
        }
    }
    return QByteArray();
}

// QStringList is an array like object, but it is not an array in QML.
// Couldn't figure out how to modify it from QML
QStringList
FoilAuth::stringListRemove(
    QStringList aList,
    const QString aString)
{
    aList.removeOne(aString);
    return aList;
}

const char*
FoilAuth::generateId(
    GString* aString)
{
    guint8 data[8];
    foil_random_generate(FOIL_RANDOM_DEFAULT, data, sizeof(data));
    g_string_append_printf(aString, "%02X%02X%02X%02X%02X%02X%02X%02X",
        data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
    return aString->str;
}

FoilOutput*
FoilAuth::createFoilFile(
    const QString aDestDir,
    GString* aOutPath)
{
    // Generate random name for the encrypted file
    FoilOutput* out = NULL;
    const QByteArray dir(aDestDir.toUtf8());

    g_string_truncate(aOutPath, 0);
    g_string_append_len(aOutPath, dir.constData(), dir.size());
    g_string_append_c(aOutPath, '/');
    const gsize prefix_len = aOutPath->len;
    for (int i = 0; i < 100 && !out; i++) {
        g_string_truncate(aOutPath, prefix_len);
        out = foil_output_file_new_open(generateId(aOutPath));
    }
    HASSERT(out);
    return out;
}

QString
FoilAuth::createEmptyFoilFile(
    const QString aDestDir)
{
    GString* dest = g_string_sized_new(aDestDir.size() + 9);
    FoilOutput* out = createFoilFile(aDestDir, dest);
    QString path;

    if (out) {
        path = QString::fromLocal8Bit(dest->str, dest->len);
        foil_output_unref(out);
    }
    g_string_free(dest, TRUE);
    return path;
}

uint
FoilAuth::TOTP(
    const QByteArray aSecret,
    quint64 aTime,
    uint aMaxPass,
    DigestAlgorithm aAlgorithm)
{
    return FoilAuth::hash(aSecret, aTime/PERIOD, aAlgorithm) % aMaxPass;
}

uint
FoilAuth::HOTP(
    const QByteArray aSecret,
    quint64 aCounter,
    uint aMaxPass,
    DigestAlgorithm aAlgorithm)
{
    return FoilAuth::hash(aSecret, aCounter, aAlgorithm) % aMaxPass;
}

uint
FoilAuth::hash(
    const QByteArray aSecret,
    quint64 aValue,
    DigestAlgorithm aAlgorithm)
{
    const guint64 msg = htobe64(aValue);
    GType (*digest_type)(void) = foil_impl_digest_sha1_get_type;
    switch (aAlgorithm) {
    case DigestAlgorithmSHA1:
        digest_type = foil_impl_digest_sha1_get_type;
        break;
    case DigestAlgorithmSHA256:
        digest_type = foil_impl_digest_sha256_get_type;
        break;
    case DigestAlgorithmSHA512:
        digest_type = foil_impl_digest_sha512_get_type;
        break;
    }
    FoilHmac* hmac = foil_hmac_new(digest_type(), aSecret.constData(), aSecret.size());
    foil_hmac_update(hmac, &msg, sizeof(msg));

    gsize hash_len;
    GBytes* hash_bytes = foil_hmac_free_to_bytes(hmac);
    const uchar* hash = (uchar*)g_bytes_get_data(hash_bytes, &hash_len);
    const uint offset = hash[hash_len - 1] & 0x0f;
    const uint mini_hash = be32toh(*(guint32*)(hash + offset)) & 0x7fffffff;

    g_bytes_unref(hash_bytes);
    return mini_hash;
}

QString
FoilAuth::toUri(
    Type aType,
    const QString aSecretBase32,
    const QString aLabel,
    QString aIssuer, int aDigits, quint64 aCounter, int aTimeShift,
    Algorithm aAlgorithm)
{
    const QByteArray secret(HarbourBase32::fromBase32(aSecretBase32));
    if (!secret.isEmpty()) {
        QString uri(FoilAuthToken((AuthType)aType, secret, aLabel, aIssuer, aDigits,
            aCounter, aTimeShift, (DigestAlgorithm) aAlgorithm).toUri());
        HDEBUG(aType << aSecretBase32 << aLabel << aIssuer << aDigits <<
            aCounter << aTimeShift << aAlgorithm << "=>" << uri);
        return uri;
    }
    return QString();
}

FoilAuthToken
FoilAuth::parseUri(
    const QString aUri)
{
    return FoilAuthToken::fromUri(aUri);
}

QList<FoilAuthToken>
FoilAuth::parseMigrationUri(
    const QString aUri)
{
    const QByteArray uri(aUri.trimmed().toUtf8());

    FoilParsePos pos;
    pos.ptr = (const guint8*)uri.constData();
    pos.end = pos.ptr + uri.size();

    QList<FoilAuthToken> result;
    FoilBytes prefixBytes;
    foil_bytes_from_string(&prefixBytes, FOILAUTH_MIGRATION_PREFIX);
    if (foil_parse_skip_bytes(&pos, &prefixBytes)) {
        // uri must be NULL-terminated
        char* unescaped = g_uri_unescape_string((char*)pos.ptr, NULL);
        if (unescaped) {
            pos.ptr = (const guint8*) unescaped;
            pos.end = pos.ptr + strlen(unescaped);
            GBytes* bytes = foil_parse_base64(&pos, FOIL_INPUT_BASE64_VALIDATE);
            if (bytes) {
                gsize size;
                gconstpointer data = g_bytes_get_data(bytes, &size);

#if HARBOUR_DEBUG
                pos.ptr = (const guint8*) g_bytes_get_data(bytes, &size);
                pos.end = pos.ptr + size;
                HDEBUG("Decoded" << size << "bytes");
                while (pos.ptr < pos.end) {
                    char line[GUTIL_HEXDUMP_BUFSIZE];
                    pos.ptr += gutil_hexdump(line, pos.ptr, pos.end - pos.ptr);
                    HDEBUG(line);
                }
#endif // HARBOUR_DEBUG

                const QByteArray buf(QByteArray::fromRawData((const char*)data, size));
                result = FoilAuthToken::fromProtoBuf(buf);
                HDEBUG(result.count() << "tokens" << result);
                g_bytes_unref(bytes);
            }
            g_free(unescaped);
        }
    }
    return result;
}

QString
FoilAuth::migrationUri(
    const QByteArray aData)
{
    QString uri;
    if (!aData.isEmpty()) {
        char* base64 = g_base64_encode((uchar*)aData.constData(), aData.size());
        char* escaped = g_uri_escape_string(base64, NULL, FALSE);
        uri = QString::fromLatin1(FOILAUTH_MIGRATION_PREFIX);
        uri.append(QLatin1String(escaped));
        g_free(escaped);
        g_free(base64);
    }
    return uri;
}

#include "FoilAuth.moc"
