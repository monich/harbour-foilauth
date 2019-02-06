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

#ifndef FOILAUTH_MODEL_H
#define FOILAUTH_MODEL_H

#include "foil_types.h"

#include <QAbstractListModel>

#include <QtQml>

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
    Q_PROPERTY(FoilState foilState READ foilState NOTIFY foilStateChanged)
    Q_DISABLE_COPY(FoilAuthModel)

    class Private;
    class ModelData;
    class BaseTask;
    class SaveInfoTask;
    class SaveTokenTask;
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

    FoilAuthModel(QObject* aParent = NULL);

    static int favoriteRole();

    int period() const;
    int timeLeft() const;
    bool busy() const;
    bool keyAvailable() const;
    bool timerActive() const;
    FoilState foilState() const;

    int indexOf(const FoilAuthToken* aToken) const;
    bool contains(const FoilAuthToken* aToken) const;
    void addToken(const FoilAuthToken* aToken, bool aFavorite);

    Q_INVOKABLE void generateKey(int aBits, QString aPassword);
    Q_INVOKABLE bool checkPassword(QString aPassword);
    Q_INVOKABLE bool changePassword(QString aOld, QString aNew);
    Q_INVOKABLE void lock(bool aTimeout);
    Q_INVOKABLE bool unlock(QString aPassword);
    Q_INVOKABLE int millisecondsLeft();

    Q_INVOKABLE bool addToken(QString aTokenBase32, QString aLabel,
        QString aIssuer, int aDigits, int aTimeShift);
    Q_INVOKABLE bool addTokenUri(QString aUri);
    Q_INVOKABLE void deleteToken(QString aId);
    Q_INVOKABLE void deleteAll();

    // QAbstractItemModel
    Qt::ItemFlags flags(const QModelIndex& aIndex) const Q_DECL_OVERRIDE;
    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& aParent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex& aIndex, int aRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex& aIndex, const QVariant& aValue, int aRole) Q_DECL_OVERRIDE;
    bool moveRows(const QModelIndex &aSrcParent, int aSrcRow, int aCount,
        const QModelIndex &aDestParent, int aDestRow) Q_DECL_OVERRIDE;

    // Callback for qmlRegisterSingletonType<FoilAuthModel>
    static QObject* createSingleton(QQmlEngine* aEngine, QJSEngine* aScript);

Q_SIGNALS:
    void countChanged();
    void busyChanged();
    void keyAvailableChanged();
    void timerActiveChanged();
    void foilStateChanged();
    void timeLeftChanged();
    void keyGenerated();
    void passwordChanged();
    void timerRestarted();

private:
    Private* iPrivate;
};

QML_DECLARE_TYPE(FoilAuthModel)

// Inline wrappers
inline bool FoilAuthModel::contains(const FoilAuthToken* aToken) const
    { return indexOf(aToken) >= 0; }

#endif // FOILAUTH_MODEL_H
