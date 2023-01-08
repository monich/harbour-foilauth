/*
 * Copyright (C) 2019-2022 Jolla Ltd.
 * Copyright (C) 2019-2023 Slava Monich <slava@monich.com>
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

#ifndef FOILAUTH_MODEL_H
#define FOILAUTH_MODEL_H

#include "FoilAuthToken.h"

#include "foil_types.h"

#include <QAbstractListModel>

class QJSEngine;
class QQmlEngine;
class FoilAuthToken;

class FoilAuthModel : public QAbstractListModel {
    Q_OBJECT
    Q_ENUMS(FoilState)
    Q_PROPERTY(int period READ period CONSTANT)
    Q_PROPERTY(int timeLeft READ timeLeft NOTIFY timeLeftChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool keyAvailable READ keyAvailable NOTIFY keyAvailableChanged)
    Q_PROPERTY(bool timerActive READ timerActive NOTIFY timerActiveChanged)
    Q_PROPERTY(QList<int> groupHeaderRows READ groupHeaderRows NOTIFY groupHeaderRowsChanged)
    Q_PROPERTY(FoilState foilState READ foilState NOTIFY foilStateChanged)
    Q_DISABLE_COPY(FoilAuthModel)

    class Util;
    class Private;
    class ModelData;
    class BaseTask;
    class SaveInfoTask;
    class GenerateKeyTask;
    class DecryptTask;
    class EncryptTask;
    class PasswordTask;

public:
    class ModelInfo;
    class DecryptAllTask;

    enum FoilState {
        FoilKeyMissing,
        FoilKeyInvalid,
        FoilKeyError,
        FoilKeyNotEncrypted,
        FoilGeneratingKey,
        FoilLocked,
        FoilLockedTimedOut,
        FoilDecrypting,
        FoilModelReady
    };

    FoilAuthModel(QObject* aParent = Q_NULLPTR);

    static int typeRole();
    static int favoriteRole();
    static int groupHeaderRole();

    int period() const;
    int timeLeft() const;
    bool busy() const;
    bool keyAvailable() const;
    bool timerActive() const;
    QList<int> groupHeaderRows() const;
    FoilState foilState() const;

    int indexOf(FoilAuthToken) const;
    bool contains(FoilAuthToken) const;
    bool containsSecret(QByteArray) const;
    void addToken(FoilAuthToken, bool aFavorite);

    Q_INVOKABLE void generateKey(int, QString);
    Q_INVOKABLE bool checkPassword(const QString);
    Q_INVOKABLE bool changePassword(const QString aOld, const QString aNew);
    Q_INVOKABLE void lock(bool aTimeout);
    Q_INVOKABLE bool unlock(const QString aPassword);

    Q_INVOKABLE void addGroup(const QString);
    Q_INVOKABLE bool addToken(int aType, const QString aTokenBase32,
        const QString aLabel, const QString aIssuer, int aDigits,
        int aCounter, int aTimeShift, int aAlgorithm);
    Q_INVOKABLE void addTokens(const QList<FoilAuthToken>);
    Q_INVOKABLE void deleteGroupItem(const QString);
    Q_INVOKABLE void deleteToken(const QString);
    Q_INVOKABLE void deleteTokens(const QStringList);
    Q_INVOKABLE QList<int> itemRowsForGroupAt(int) const;
    Q_INVOKABLE QStringList getIdsAt(const QList<int>) const;
    Q_INVOKABLE QStringList generateMigrationUris(const QList<int>) const;

    // QAbstractItemModel
    Qt::ItemFlags flags(const QModelIndex&) const Q_DECL_OVERRIDE;
    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& aParent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex&, int) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex&, const QVariant&, int) Q_DECL_OVERRIDE;
    bool moveRows(const QModelIndex&, int, int, const QModelIndex&, int) Q_DECL_OVERRIDE;

    // Callback for qmlRegisterSingletonType<FoilAuthModel>
    static QObject* createSingleton(QQmlEngine*, QJSEngine*);

Q_SIGNALS:
    void countChanged();
    void busyChanged();
    void keyAvailableChanged();
    void timerActiveChanged();
    void groupHeaderRowsChanged();
    void foilStateChanged();
    void timeLeftChanged();
    void keyGenerated();
    void passwordChanged();
    void timerRestarted();

private:
    Private* iPrivate;
};

// Inline wrappers
inline bool FoilAuthModel::contains(const FoilAuthToken aToken) const
    { return indexOf(aToken) >= 0; }

#endif // FOILAUTH_MODEL_H
