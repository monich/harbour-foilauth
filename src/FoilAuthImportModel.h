/*
 * Copyright (C) 2022 Jolla Ltd.
 * Copyright (C) 2022 Slava Monich <slava@monich.com>
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

#ifndef FOILAUTH_IMPORT_MODEL_H
#define FOILAUTH_IMPORT_MODEL_H

#include "FoilAuthToken.h"

#include <QAbstractListModel>

class FoilAuthImportModel :
    public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY(FoilAuthImportModel)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QList<FoilAuthToken> selectedTokens READ selectedTokens NOTIFY selectedTokensChanged)
    Q_PROPERTY(bool haveSelectedTokens READ haveSelectedTokens NOTIFY haveSelectedTokensChanged)

public:
    FoilAuthImportModel(QObject* aParent = Q_NULLPTR);
    ~FoilAuthImportModel();

    QList<FoilAuthToken> selectedTokens() const;
    bool haveSelectedTokens() const;

    Q_INVOKABLE void setUri(const QString);
    Q_INVOKABLE void setToken(FoilAuthToken);
    Q_INVOKABLE void setTokens(const QList<FoilAuthToken>);

    Q_INVOKABLE QVariantMap getToken(int) const;
    Q_INVOKABLE QList<FoilAuthToken> getTokens() const;
    Q_INVOKABLE void setToken(int, int, int, const QString, const QString, const QString, int, int, int);

    // QAbstractItemModel
    Qt::ItemFlags flags(const QModelIndex&) const Q_DECL_OVERRIDE;
    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& aParent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex&, int) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex&, const QVariant&, int) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void countChanged();
    void selectedTokensChanged();
    void haveSelectedTokensChanged();

private:
    class ModelData;
    class Private;
    Private* iPrivate;
};

#endif // FOILAUTH_IMPORT_MODEL_H
