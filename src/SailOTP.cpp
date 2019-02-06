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

#include "SailOTP.h"

#include "FoilAuthModel.h"
#include "FoilAuthToken.h"

#include "HarbourBase32.h"
#include "HarbourDebug.h"

#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

// ==========================================================================
// SailOTP::Private
// ==========================================================================

class SailOTP::Private {
public:
    class Token {
    public:
        Token() : iFavorite(false) {}

    public:
        FoilAuthToken iToken;
        bool iFavorite;
    };

    static const QString DB_TYPE;
    static const QString DB_NAME;
    static const QString DB_TABLE;

    static const QString COL_TITLE;
    static const QString COL_SECRET;
    static const QString COL_TYPE;
    static const QString COL_COUNTER;
    static const QString COL_FAV;
    static const QString COL_SORT;
    static const QString COL_LEN;
    static const QString COL_DIFF;

    static QString databaseDir();
    static QString databasePath();

    Private();

    void fetchTokens();

public:
    QList<Token> iTokens;
};

#define TABLE_NAME "OTPStorage"
const QString SailOTP::Private::DB_TYPE("QSQLITE");
const QString SailOTP::Private::DB_NAME("harbour-sailotp");
const QString SailOTP::Private::DB_TABLE(TABLE_NAME);

#define SORT_COL "sort"
const QString SailOTP::Private::COL_TITLE("title");     // TEXT
const QString SailOTP::Private::COL_SECRET("secret");   // TEXT
const QString SailOTP::Private::COL_TYPE("type");       // TEXT
const QString SailOTP::Private::COL_COUNTER("counter"); // INTEGER
const QString SailOTP::Private::COL_FAV("fav");         // INTEGER
const QString SailOTP::Private::COL_SORT(SORT_COL);     // INTEGER
const QString SailOTP::Private::COL_LEN("len");         // INTEGER
const QString SailOTP::Private::COL_DIFF("diff");       // INTEGER

QString SailOTP::Private::databaseDir()
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        "/harbour-sailotp/harbour-sailotp/QML/OfflineStorage/Databases");
    return dir.path();
}

QString SailOTP::Private::databasePath()
{
    // This is how LocalStorage plugin generates database file name
    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(DB_NAME.toUtf8());
    const QString dbId(QLatin1String(md5.result().toHex()));
    return databaseDir() + "/" + dbId + ".sqlite";
}

SailOTP::Private::Private()
{
    // SqlDatabase object needs to go out of scope before we can
    // call QSqlDatabase::removeDatabase
    fetchTokens();
    QSqlDatabase::removeDatabase(DB_NAME);
}

void SailOTP::Private::fetchTokens()
{
    QSqlDatabase db(QSqlDatabase::database(Private::DB_NAME));
    if (!db.isValid()) {
        HDEBUG("adding database" << DB_NAME);
        db = QSqlDatabase::addDatabase(DB_TYPE, DB_NAME);
    }
    const QString path(databasePath());
    db.setDatabaseName(path);
    if (db.open()) {
        HDEBUG("Opened " << qPrintable(path));
        QSqlQuery query(db);
        if (query.exec("SELECT * FROM " TABLE_NAME " ORDER BY " SORT_COL) ||
            query.exec("SELECT * FROM " TABLE_NAME)) {
            while (query.next()) {
                const QString type(query.value(COL_TYPE).toString());
                if (type.compare(FoilAuthToken::TYPE_TOTP, Qt::CaseInsensitive) == 0) {
                    const QString base32(query.value(COL_SECRET).toString());
                    const QByteArray secret(HarbourBase32::fromBase32(base32));
                    if (!secret.isEmpty()) {
                        bool ok;
                        int value;

                        Token token;
                        token.iToken.iBytes = secret;
                        token.iToken.iLabel = query.value(COL_TITLE).toString();
                        value = query.value(COL_FAV).toInt(&ok);
                        if (ok) token.iFavorite = value != 0;
                        value = query.value(COL_LEN).toInt(&ok);
                        if (ok && value > 0) token.iToken.iDigits = value;
                        value = query.value(COL_DIFF).toInt(&ok);
                        if (ok) token.iToken.iTimeShift = value;
                        HDEBUG(token.iToken.iLabel << base32 << token.iToken.iDigits);
                        iTokens.append(token);
                    }
                }
            }
        } else {
            HWARN(query.lastError());
        }
    }
}

// ==========================================================================
// SailOTP
// ==========================================================================

SailOTP::SailOTP(QObject* aParent) :
    QObject(aParent),
    iPrivate(Q_NULLPTR)
{
}

SailOTP::~SailOTP()
{
    delete iPrivate;
}

// Callback for qmlRegisterSingletonType<SailOTP>
QObject* SailOTP::createSingleton(QQmlEngine* aEngine, QJSEngine* aScript)
{
    return new SailOTP(aEngine);
}

int SailOTP::fetchTokens()
{
    delete iPrivate;
    iPrivate = new Private;
    if (iPrivate->iTokens.isEmpty()) {
        delete iPrivate;
        iPrivate = Q_NULLPTR;
        return 0;
    } else {
        HDEBUG("Fetched" << iPrivate->iTokens.count() << "token(s)");
        return iPrivate->iTokens.count();
    }
}

void SailOTP::importTokens(QObject* aDestModel)
{
    if (iPrivate) {
        FoilAuthModel* model = qobject_cast<FoilAuthModel*>(aDestModel);
        if (model) {
            const int n = iPrivate->iTokens.count();
            for (int i = 0; i < n; i++) {
                const Private::Token& token(iPrivate->iTokens.at(i));
                const FoilAuthToken* authToken = &token.iToken;
                if (model->contains(authToken)) {
                    HDEBUG(token.iToken.iLabel << "is already there");
                } else {
                    model->addToken(authToken, token.iFavorite);
                }
            }
        }
        delete iPrivate;
        iPrivate = Q_NULLPTR;
    }
}

void SailOTP::dropTokens()
{
    delete iPrivate;
    iPrivate = Q_NULLPTR;
}
