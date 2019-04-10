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

#include "FoilAuth.h"
#include "FoilAuthToken.h"

#include "HarbourBase32.h"
#include "HarbourDebug.h"

#include "foil_hmac.h"
#include "foil_digest.h"
#include "foil_random.h"
#include "foil_output.h"

#include <QStringList>
#include <QFile>
#include <QFileSystemWatcher>
#include <QQmlEngine>

#define FOILAPPS_DIR    "/usr/bin"
#define FOILPICS_PATH   FOILAPPS_DIR "/harbour-foilpics"
#define FOILNOTES_PATH  FOILAPPS_DIR "/harbour-foilnotes"

// ==========================================================================
// FoilAuth::Private
// ==========================================================================

class FoilAuth::Private : public QObject {
    Q_OBJECT

public:
    Private(FoilAuth* aParent);

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

inline FoilAuth* FoilAuth::Private::foilAuth() const
{
    return qobject_cast<FoilAuth*>(parent());
}

bool FoilAuth::Private::foilPicsInstalled()
{
    const bool installed = QFile::exists(FOILPICS_PATH);
    HDEBUG("FoilPics is" << (installed ? "installed" : "not installed"));
    return installed;
}

bool FoilAuth::Private::foilNotesInstalled()
{
    const bool installed = QFile::exists(FOILNOTES_PATH);
    HDEBUG("FoilNotes is" << (installed ? "installed" : "not installed"));
    return installed;
}

bool FoilAuth::Private::otherFoilAppsInstalled()
{
    return foilPicsInstalled() || foilNotesInstalled();
}

void FoilAuth::Private::checkFoilAppsInstalled()
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
QObject* FoilAuth::createSingleton(QQmlEngine* aEngine, QJSEngine* aScript)
{
    return new FoilAuth(aEngine);
}

bool FoilAuth::otherFoilAppsInstalled() const
{
    return iPrivate->iOtherFoilAppsInstalled;
}

QSize FoilAuth::toSize(QVariant aVariant)
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

bool FoilAuth::isValidBase32(QString aBase32)
{
    return HarbourBase32::isValidBase32(aBase32);
}

QByteArray FoilAuth::fromBase32(QString aBase32)
{
    return HarbourBase32::fromBase32(aBase32);
}

QString FoilAuth::toBase32(QByteArray aBinary, bool aLowerCase)
{
    return HarbourBase32::toBase32(aBinary, aLowerCase);
}

QByteArray FoilAuth::toByteArray(GBytes* aData)
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

FoilOutput* FoilAuth::createFoilFile(QString aDestDir, GString* aOutPath)
{
    // Generate random name for the encrypted file
    FoilOutput* out = NULL;
    const QByteArray dir(aDestDir.toUtf8());

    g_string_truncate(aOutPath, 0);
    g_string_append_len(aOutPath, dir.constData(), dir.size());
    g_string_append_c(aOutPath, '/');
    const gsize prefix_len = aOutPath->len;
    for (int i = 0; i < 100 && !out; i++) {
        guint8 data[8];
        foil_random_generate(FOIL_RANDOM_DEFAULT, data, sizeof(data));
        g_string_truncate(aOutPath, prefix_len);
        g_string_append_printf(aOutPath, "%02X%02X%02X%02X%02X%02X%02X%02X",
            data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
        out = foil_output_file_new_open(aOutPath->str);
    }
    HASSERT(out);
    return out;
}

QString FoilAuth::createEmptyFoilFile(QString aDestDir)
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

uint FoilAuth::TOTP(QByteArray aSecret, quint64 aTime, uint aMaxPass)
{
    const guint64 msg = htobe64(aTime/PERIOD);
    FoilHmac* hmac = foil_hmac_new(FOIL_DIGEST_SHA1, aSecret.constData(), aSecret.size());
    foil_hmac_update(hmac, &msg, sizeof(msg));

    gsize hash_len;
    GBytes* hash_bytes = foil_hmac_free_to_bytes(hmac);
    const uchar* hash = (uchar*)g_bytes_get_data(hash_bytes, &hash_len);
    const uint offset = hash[hash_len - 1] & 0x0f;
    const uint mini_hash = be32toh(*(guint32*)(hash + offset)) & 0x7fffffff;

    g_bytes_unref(hash_bytes);
    return mini_hash % aMaxPass;
}

QString FoilAuth::tokenQrCode(QString aSecretBase32, QString aLabel,
    QString aIssuer, int aDigits, int aTimeShift)
{
    QByteArray secret(fromBase32(aSecretBase32));
    if (!secret.isEmpty()) {
        HDEBUG(aSecretBase32 << aLabel << aIssuer << aDigits << aTimeShift);
        FoilAuthToken token(secret, aLabel, aIssuer, aDigits, aTimeShift);
        QByteArray qrcode(token.toQrCode());
        if (!qrcode.isEmpty()) {
            QString base32(toBase32(qrcode));
            HDEBUG(base32);
            return base32;
        }
    }
    return QString();
}

QVariantMap FoilAuth::parseUri(QString aUri)
{
    FoilAuthToken token;
    token.parseUri(aUri);
    return token.toVariantMap();
}

#include "FoilAuth.moc"
