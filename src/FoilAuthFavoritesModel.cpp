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

#include "FoilAuthFavoritesModel.h"
#include "FoilAuthModel.h"

#include "HarbourDebug.h"

// ==========================================================================
// FoilAuthFavoritesModel::Private
// ==========================================================================

class FoilAuthFavoritesModel::Private : public QObject {
    Q_OBJECT
public:
    Private(FoilAuthFavoritesModel* aParent);

    FoilAuthFavoritesModel* parentModel();

public Q_SLOTS:
    void checkCount();

public:
    int iStartIndex;
    int iLastKnownCount;
};

FoilAuthFavoritesModel::Private::Private(FoilAuthFavoritesModel* aParent) :
    QObject(aParent),
    iStartIndex(0),
    iLastKnownCount(0)
{
    connect(aParent, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(checkCount()));
    connect(aParent, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(checkCount()));
    connect(aParent, SIGNAL(modelReset()), SLOT(checkCount()));
}

inline FoilAuthFavoritesModel* FoilAuthFavoritesModel::Private::parentModel()
{
    return qobject_cast<FoilAuthFavoritesModel*>(parent());
}

void FoilAuthFavoritesModel::Private::checkCount()
{
    FoilAuthFavoritesModel* model = parentModel();
    const int count = model->rowCount();
    if (iLastKnownCount != count) {
        iLastKnownCount = count;
        Q_EMIT model->countChanged();
    }
}

// ==========================================================================
// FoilAuthFavoritesModel
// ==========================================================================

#define SUPER QSortFilterProxyModel

FoilAuthFavoritesModel::FoilAuthFavoritesModel(QObject* aParent) :
    SUPER(aParent),
    iPrivate(new Private(this))
{
}

void FoilAuthFavoritesModel::setSourceModelObject(QObject* aModel)
{
    setSourceModel(qobject_cast<QAbstractItemModel*>(aModel));
}

bool FoilAuthFavoritesModel::filterAcceptsRow(int aSourceRow,
    const QModelIndex& aParent) const
{
    const QAbstractItemModel* model = sourceModel();
    const QModelIndex index = model->index(aSourceRow, 0, aParent);
    return model->data(index, FoilAuthModel::favoriteRole()).toBool();
}

QModelIndex FoilAuthFavoritesModel::mapToSource(const QModelIndex& aIndex) const
{
    return SUPER::mapToSource(aIndex);
}

QModelIndex FoilAuthFavoritesModel::mapFromSource(const QModelIndex& aIndex) const
{
    return SUPER::mapFromSource(aIndex);
}

int FoilAuthFavoritesModel::startIndex() const
{
    return iPrivate->iStartIndex;
}

void FoilAuthFavoritesModel::setStartIndex(int aIndex)
{
    HDEBUG(aIndex);
    if (iPrivate->iStartIndex != aIndex) {
        const QAbstractItemModel* model = sourceModel();
        if (model) {
            const int count = model->rowCount();
            const int oldRealIndex = qBound(0, iPrivate->iStartIndex, count);
            const int newRealIndex = qBound(0, aIndex, count);
            iPrivate->iStartIndex = aIndex;
            if (oldRealIndex != newRealIndex) {
            }
        } else {
            iPrivate->iStartIndex = aIndex;
        }
        Q_EMIT startIndexChanged();
    }
}

int FoilAuthFavoritesModel::count() const
{
    return rowCount();
}

#include "FoilAuthFavoritesModel.moc"
