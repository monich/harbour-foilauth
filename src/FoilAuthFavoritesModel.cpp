/*
 * Copyright (C) 2019-2020 Jolla Ltd.
 * Copyright (C) 2019-2020 Slava Monich <slava@monich.com>
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
#include "FoilAuth.h"

#include "HarbourDebug.h"

// ==========================================================================
// FoilAuthFavoritesModel::Private
// ==========================================================================

class FoilAuthFavoritesModel::Private : public QObject {
    Q_OBJECT
public:
    Private(FoilAuthFavoritesModel* aParent);

    FoilAuthFavoritesModel* parentModel() const;
    bool needTimer() const;
    void updateNeedTimer();

public Q_SLOTS:
    void checkCount();
    void onDataChanged(const QModelIndex& aTopLeft, const QModelIndex& aBottomRight,
        const QVector<int>& aRoles);

public:
    int iLastKnownCount;
    bool iNeedTimer;
};

FoilAuthFavoritesModel::Private::Private(FoilAuthFavoritesModel* aParent) :
    QObject(aParent),
    iLastKnownCount(0),
    iNeedTimer(needTimer())
{
    connect(aParent, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(checkCount()));
    connect(aParent, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(checkCount()));
    connect(aParent, SIGNAL(modelReset()), SLOT(checkCount()));
    connect(aParent, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),
        SLOT(onDataChanged(QModelIndex,QModelIndex,QVector<int>)));
}

inline FoilAuthFavoritesModel* FoilAuthFavoritesModel::Private::parentModel() const
{
    return qobject_cast<FoilAuthFavoritesModel*>(parent());
}

bool FoilAuthFavoritesModel::Private::needTimer() const
{
    FoilAuthFavoritesModel* model = parentModel();
    const int count = model->rowCount();
    for (int row = 0; row < count; row++) {
        bool ok;
        const int type = model->data(model->index(row, 0),
            FoilAuthModel::typeRole()).toInt(&ok);
        if (ok && type == FoilAuth::TypeTOTP) {
            return true;
        }
    }
    return false;
}

void FoilAuthFavoritesModel::Private::checkCount()
{
    FoilAuthFavoritesModel* model = parentModel();
    const int count = model->rowCount();
    if (iLastKnownCount != count) {
        iLastKnownCount = count;
        Q_EMIT model->countChanged();
    }
    updateNeedTimer();
}

void FoilAuthFavoritesModel::Private::onDataChanged(const QModelIndex& aTopLeft,
    const QModelIndex& aBottomRight, const QVector<int>& aRoles)
{
    if (aRoles.isEmpty() || aRoles.contains(FoilAuthModel::typeRole())) {
        updateNeedTimer();
    }
}

void FoilAuthFavoritesModel::Private::updateNeedTimer()
{
    const bool need = needTimer();
    if (iNeedTimer != need) {
        iNeedTimer = need;
        Q_EMIT parentModel()->needTimerChanged();
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
    connect(this, SIGNAL(sourceModelChanged()), SIGNAL(sourceModelObjectChanged()));
}

void FoilAuthFavoritesModel::setSourceModelObject(QObject* aModel)
{
    setSourceModel(qobject_cast<QAbstractItemModel*>(aModel));
}

bool FoilAuthFavoritesModel::needTimer() const
{
    return iPrivate->iNeedTimer;
}

bool FoilAuthFavoritesModel::filterAcceptsRow(int aSourceRow,
    const QModelIndex& aParent) const
{
    const QAbstractItemModel* model = sourceModel();
    const QModelIndex index = model->index(aSourceRow, 0, aParent);
    return model->data(index, FoilAuthModel::favoriteRole()).toBool();
}

#include "FoilAuthFavoritesModel.moc"
