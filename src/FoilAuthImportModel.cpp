/*
 * Copyright (C) 2022-2023 Slava Monich <slava@monich.com>
 * Copyright (C) 2022 Jolla Ltd.
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

#include "FoilAuthImportModel.h"
#include "FoilAuth.h"

#include "HarbourBase32.h"
#include "HarbourDebug.h"

#include <gutil_misc.h>

#include <foil_input.h>
#include <foil_util.h>

#include <QUrl>

// Model roles
#define MODEL_ROLES_(first,role,last) \
    first(Type,type) \
    role(Algorithm,algorithm) \
    role(Label,label) \
    role(Issuer,issuer) \
    role(Secret,secret) \
    role(Digits,digits) \
    role(Counter,counter) \
    role(Timeshift,timeshift) \
    last(Selected,selected)

#define MODEL_ROLES(role) \
    MODEL_ROLES_(role,role,role)

// ==========================================================================
// FoilAuthImportModel::ModelData
// ==========================================================================

class FoilAuthImportModel::ModelData
{
public:
    typedef QList<ModelData*> List;

    enum Role {
#define FIRST(X,x) FirstRole = Qt::UserRole, X##Role = FirstRole,
#define ROLE(X,x) X##Role,
#define LAST(X,x) X##Role, LastRole = X##Role
        MODEL_ROLES_(FIRST,ROLE,LAST)
#undef FIRST
#undef ROLE
#undef LAST
    };

    ModelData(FoilAuthToken);

    QVariant get(Role) const;

public:
    bool iSelected;
    FoilAuthToken iToken;
};

FoilAuthImportModel::ModelData::ModelData(
    FoilAuthToken aToken) :
    iSelected(true),
    iToken(aToken)
{
}

QVariant
FoilAuthImportModel::ModelData::get(
    Role aRole) const
{
    switch (aRole) {
    case TypeRole: return (int) iToken.type();
    case AlgorithmRole: return (int) iToken.algorithm();
    case LabelRole: return iToken.label();
    case IssuerRole: return iToken.issuer();
    case SecretRole: return iToken.secretBase32();
    case DigitsRole: return iToken.digits();
    case CounterRole: return iToken.counter();
    case TimeshiftRole: return iToken.timeshift();
    case SelectedRole: return iSelected;
    }
    return QVariant();
}

// ==========================================================================
// FoilAuthImportModel::Private
// ==========================================================================

class FoilAuthImportModel::Private
{
public:
    Private(FoilAuthImportModel*);
    ~Private();

    ModelData* dataAt(int);
    void setItems(const ModelData::List);
    void updateSelectedTokens();

public:
    FoilAuthImportModel* iModel;
    ModelData::List iList;
    QList<FoilAuthToken> iSelectedTokens;
};

FoilAuthImportModel::Private::Private(
    FoilAuthImportModel* aModel) :
    iModel(aModel)
{
}

FoilAuthImportModel::Private::~Private()
{
    qDeleteAll(iList);
}

inline
FoilAuthImportModel::ModelData*
FoilAuthImportModel::Private::dataAt(
    int aIndex)
{
    return (aIndex >= 0 && aIndex < iList.count()) ?
        iList.at(aIndex) : Q_NULLPTR;
}

void
FoilAuthImportModel::Private::setItems(
    const ModelData::List aList)
{
    const int prevCount = iList.count();
    const int newCount = aList.count();
    const int changed = qMin(prevCount, newCount);
    const QList<FoilAuthToken> prevSelectedTokens(iSelectedTokens);

    if (newCount < prevCount) {
        iModel->beginRemoveRows(QModelIndex(), newCount, prevCount - 1);
        qDeleteAll(iList);
        iList = aList;
        iModel->endRemoveRows();
    } else if (newCount > prevCount) {
        iModel->beginInsertRows(QModelIndex(), prevCount, newCount - 1);
        qDeleteAll(iList);
        iList = aList;
        iModel->endInsertRows();
    } else {
        qDeleteAll(iList);
        iList = aList;
    }

    updateSelectedTokens();
    if (prevSelectedTokens != iSelectedTokens) {
        if (!prevSelectedTokens.count() != !iSelectedTokens.count()) {
            Q_EMIT iModel->haveSelectedTokensChanged();
        }
        Q_EMIT iModel->selectedTokensChanged();
    }
    if (changed > 0) {
        Q_EMIT iModel->dataChanged(iModel->index(0), iModel->index(changed - 1));
    }
}

void
FoilAuthImportModel::Private::updateSelectedTokens()
{
    QList<FoilAuthToken> list;
    const int n = iList.count();

    for (int i = 0; i < n; i++) {
        const ModelData* entry = iList.at(i);

        if (entry->iSelected) {
            list.append(entry->iToken);
        }
    }

    HDEBUG("selected" << list);
    iSelectedTokens = list;
}

// ==========================================================================
// FoilAuthImportModel
// ==========================================================================

FoilAuthImportModel::FoilAuthImportModel(
    QObject* aParent) :
    QAbstractListModel(aParent),
    iPrivate(new Private(this))
{
    connect(this, SIGNAL(modelReset()), SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsInserted(QModelIndex,int,int)), SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)), SIGNAL(countChanged()));
}

FoilAuthImportModel::~FoilAuthImportModel()
{
    delete iPrivate;
}

void
FoilAuthImportModel::setUri(
    const QString aUri)
{
    const QByteArray uri(aUri.trimmed().toUtf8());

    HDEBUG(uri.constData());

    ModelData::List items;
    FoilAuthToken singleToken(FoilAuth::parseUri(uri));

    if (singleToken.isValid()) {
        items.append(new ModelData(singleToken));
        HDEBUG("single token" << singleToken);
    } else {
        const QList<FoilAuthToken> tokens(FoilAuth::parseMigrationUri(uri));
        const int n = tokens.count();

        for (int i = 0; i < n; i++) {
            items.append(new ModelData(tokens.at(i)));
        }
    }

    iPrivate->setItems(items);
}

void
FoilAuthImportModel::setToken(
    FoilAuthToken aToken)
{
    ModelData::List items;

    HDEBUG(aToken);
    if (aToken.isValid()) {
        items.append(new ModelData(aToken));
    }

    iPrivate->setItems(items);
}

void
FoilAuthImportModel::setTokens(
    const QList<FoilAuthToken> aTokens)
{
    ModelData::List items;
    const int n = aTokens.count();

    HDEBUG(aTokens);
    for (int i = 0; i < n; i++) {
        FoilAuthToken token(aTokens.at(i));

        if (token.isValid()) {
            items.append(new ModelData(token));
        }
    }

    iPrivate->setItems(items);
}

QVariantMap
FoilAuthImportModel::getToken(
    int aIndex) const
{
    const ModelData* entry = iPrivate->dataAt(aIndex);

    return entry ? entry->iToken.toVariantMap() : QVariantMap();
}

QList<FoilAuthToken>
FoilAuthImportModel::getTokens() const
{
    QList<FoilAuthToken> tokens;
    const int n = iPrivate->iList.count();

    tokens.reserve(n);
    for (int i = 0; i < n; i++) {
        tokens.append(iPrivate->iList.at(i)->iToken);
    }
    return tokens;
}

QList<FoilAuthToken>
FoilAuthImportModel::selectedTokens() const
{
    return iPrivate->iSelectedTokens;
}

bool
FoilAuthImportModel::haveSelectedTokens() const
{
    return !iPrivate->iSelectedTokens.isEmpty();
}

Qt::ItemFlags
FoilAuthImportModel::flags(
    const QModelIndex& aIndex) const
{
    return QAbstractListModel::flags(aIndex) | Qt::ItemIsEditable;
}

QHash<int,QByteArray>
FoilAuthImportModel::roleNames() const
{
    QHash<int,QByteArray> roles;
#define ROLE(X,x) roles.insert(ModelData::X##Role, #x);
MODEL_ROLES(ROLE)
#undef ROLE
    return roles;
}

int
FoilAuthImportModel::rowCount(
    const QModelIndex& aParent) const
{
    return iPrivate->iList.count();
}

QVariant
FoilAuthImportModel::data(
    const QModelIndex& aIndex,
    int aRole) const
{
    const ModelData* entry = iPrivate->dataAt(aIndex.row());

    return entry ? entry->get((ModelData::Role)aRole) : QVariant();
}

bool
FoilAuthImportModel::setData(
    const QModelIndex& aIndex,
    const QVariant& aValue,
    int aRole)
{
    ModelData* entry = iPrivate->dataAt(aIndex.row());
    bool ok = false;

    if (entry) {
        QVector<int> roles;
        QString s;
        bool b;
        int i;
        uint u;

        switch ((ModelData::Role)aRole) {
        case ModelData::TypeRole:
            i = aValue.toInt(&ok);
            if (ok) {
                const FoilAuthTypes::AuthType type = FoilAuthToken::validType(i);

                HDEBUG(aIndex.row() << "type" << type);
                if (entry->iToken.type() != type) {
                    entry->iToken = entry->iToken.withType(type);
                    roles.append(aRole);
                }
            }
            break;

        case ModelData::AlgorithmRole:
            i = aValue.toInt(&ok);
            if (ok) {
                const FoilAuthTypes::DigestAlgorithm alg = FoilAuthToken::validAlgorithm(i);

                HDEBUG(aIndex.row() << "algorithm" << alg);
                if (entry->iToken.algorithm() != alg) {
                    entry->iToken = entry->iToken.withAlgorithm(alg);
                    roles.append(aRole);
                }
            }
            break;

        case ModelData::LabelRole:
            s = aValue.toString();
            HDEBUG(aIndex.row() << "label" << s);
            if (entry->iToken.label() != s) {
                entry->iToken = entry->iToken.withLabel(s);
                roles.append(aRole);
            }
            ok = true;
            break;

        case ModelData::IssuerRole:
            s = aValue.toString();
            HDEBUG(aIndex.row() << "issuer" << s);
            if (entry->iToken.issuer() != s) {
                entry->iToken = entry->iToken.withIssuer(s);
                roles.append(aRole);
            }
            ok = true;
            break;

        case ModelData::SecretRole:
            s = aValue.toString().trimmed().toLower();
            ok = HarbourBase32::isValidBase32(s);
            if (ok) {
                const QByteArray secret(HarbourBase32::fromBase32(s));

                HDEBUG(aIndex.row() << "secret" << s);
                if (entry->iToken.secret() != secret) {
                    entry->iToken = entry->iToken.withSecret(secret);
                    roles.append(aRole);
                }
            }
            break;

        case ModelData::DigitsRole:
            i = aValue.toInt(&ok);
            if (ok) {
                if (i >= FoilAuthTypes::MIN_DIGITS &&
                    i <= FoilAuthTypes::MAX_DIGITS) {
                    HDEBUG(aIndex.row() << "digits" << i);
                    if (entry->iToken.digits() != i) {
                        entry->iToken = entry->iToken.withDigits(i);
                        roles.append(aRole);
                    }
                } else {
                    ok = false;
                }
            }
            break;

        case ModelData::CounterRole:
            u = aValue.toUInt(&ok);
            if (ok) {
                HDEBUG(aIndex.row() << "counter" << u);
                if (entry->iToken.counter() != u) {
                    entry->iToken = entry->iToken.withCounter(u);
                    roles.append(aRole);
                }
            }
            break;

        case ModelData::TimeshiftRole:
            i = aValue.toInt(&ok);
            if (ok) {
                HDEBUG(aIndex.row() << "timeshift" << i);
                if (entry->iToken.timeshift() != i) {
                    entry->iToken = entry->iToken.withTimeshift(i);
                    roles.append(aRole);
                }
            }
            break;

        case ModelData::SelectedRole:
            b = aValue.toBool();
            HDEBUG(aIndex.row() << "selected" << b);
            if (entry->iSelected != b) {
                entry->iSelected = b;
                roles.append(aRole);

                // Selection has changed, update the list of selected tokens
                const bool hadSelectedTokens = haveSelectedTokens();
                iPrivate->updateSelectedTokens();
                if (hadSelectedTokens != haveSelectedTokens()) {
                    Q_EMIT haveSelectedTokensChanged();
                }
                // If the entry is selected, the signal will be emitted from
                // common place below
                if (!entry->iSelected) {
                    Q_EMIT selectedTokensChanged();
                }
            }
            ok = true;
            break;
        }

        if (!roles.isEmpty()) {
            if (entry->iSelected) {
                iPrivate->updateSelectedTokens();
                Q_EMIT selectedTokensChanged();
            }
            Q_EMIT dataChanged(aIndex, aIndex, roles);
        }
    }
    return ok;
}

void
FoilAuthImportModel::setToken(
    int aRow,
    int aType,
    int aAlgorithm,
    const QString aLabel,
    const QString aIssuer,
    const QString aSecretBase32,
    int aDigits,
    int aCounter,
    int aTimeshift)
{
    ModelData* entry = iPrivate->dataAt(aRow);

    if (entry) {
        QVector<int> roles;
        const FoilAuthTypes::AuthType type = FoilAuthToken::validType(aType);
        const FoilAuthTypes::DigestAlgorithm alg = FoilAuthToken::validAlgorithm(aAlgorithm);
        const QString secret(aSecretBase32.trimmed());

        if (type != entry->iToken.type()) {
            entry->iToken = entry->iToken.withType(type);
            HDEBUG(aRow << "type" << type);
            roles.append(ModelData::TypeRole);
        }

        if (alg != entry->iToken.algorithm()) {
            entry->iToken = entry->iToken.withAlgorithm(alg);
            HDEBUG(aRow << "algorithm" << alg);
            roles.append(ModelData::AlgorithmRole);
        }

        if (entry->iToken.label() != aLabel) {
            entry->iToken = entry->iToken.withLabel(aLabel);
            HDEBUG(aRow << "label" << aLabel);
            roles.append(ModelData::LabelRole);
        }

        if (entry->iToken.issuer() != aIssuer) {
            entry->iToken = entry->iToken.withIssuer(aIssuer);
            HDEBUG(aRow << "issuer" << aIssuer);
            roles.append(ModelData::IssuerRole);
        }

        if (HarbourBase32::isValidBase32(secret)) {
            const QByteArray bytes(HarbourBase32::fromBase32(secret));

            if (entry->iToken.secret() != bytes) {
                entry->iToken = entry->iToken.withSecret(bytes);
                HDEBUG(aRow << "secret" << entry->iToken.secretBase32());
                roles.append(ModelData::SecretRole);
            }
        }

        if (aDigits != entry->iToken.digits() &&
            aDigits >= FoilAuthTypes::MIN_DIGITS &&
            aDigits <= FoilAuthTypes::MAX_DIGITS) {
            entry->iToken = entry->iToken.withDigits(aDigits);
            HDEBUG(aRow << "digits" << aDigits);
            roles.append(ModelData::DigitsRole);
        }

        if ((int) entry->iToken.counter() != aCounter) {
            entry->iToken = entry->iToken.withCounter(aCounter);
            HDEBUG(aRow << "counter" << aCounter);
            roles.append(ModelData::CounterRole);
        }

        if (entry->iToken.timeshift() != aTimeshift) {
            entry->iToken = entry->iToken.withTimeshift(aTimeshift);
            HDEBUG(aRow << "timeshift" << aTimeshift);
            roles.append(ModelData::TimeshiftRole);
        }

        if (!roles.isEmpty()) {
            if (entry->iSelected) {
                iPrivate->updateSelectedTokens();
                Q_EMIT selectedTokensChanged();
            }
            const QModelIndex idx(index(aRow));
            Q_EMIT dataChanged(idx, idx, roles);
        }
    }
}
