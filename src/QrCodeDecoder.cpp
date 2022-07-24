/*
 * Copyright (C) 2019-2022 Jolla Ltd.
 * Copyright (C) 2019-2022 Slava Monich <slava@monich.com>
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

#include "QrCodeDecoder.h"

#include "HarbourDebug.h"

#include <QAtomicInt>

#include "zbar/QZBarImage.h"

// ==========================================================================
// QrCodeDecoder::Result::Private
// ==========================================================================

class QrCodeDecoder::Result::Private {
public:
    Private(const QString, const QList<QPointF>, const QString);

public:
    QAtomicInt iRef;
    const QString iText;
    const QList<QPointF> iPoints;
    const QString iFormatName;
};

QrCodeDecoder::Result::Private::Private(
    const QString aText,
    const QList<QPointF> aPoints,
    const QString formatName) :
    iRef(1),
    iText(aText),
    iPoints(aPoints),
    iFormatName(formatName)
{
}

// ==========================================================================
// QrCodeDecoder::Result
// ==========================================================================

QrCodeDecoder::Result::Result(
    const QString aText,
    const QList<QPointF> aPoints,
    const QString formatName) :
    iPrivate(new Private(aText, aPoints, formatName))
{
}

QrCodeDecoder::Result::Result(
    const Result& aResult) :
    iPrivate(aResult.iPrivate)
{
    if (iPrivate) {
        iPrivate->iRef.ref();
    }
}

QrCodeDecoder::Result::Result() :
    iPrivate(NULL)
{
}

QrCodeDecoder::Result::~Result()
{
    if (iPrivate && !iPrivate->iRef.deref()) {
        delete iPrivate;
    }
}

QrCodeDecoder::Result&
QrCodeDecoder::Result::operator=(
    const Result& aResult)
{
    if (iPrivate != aResult.iPrivate) {
        if (iPrivate && !iPrivate->iRef.deref()) {
            delete iPrivate;
        }
        iPrivate = aResult.iPrivate;
        if (iPrivate) {
            iPrivate->iRef.ref();
        }
    }
    return *this;
}

bool
QrCodeDecoder::Result::isValid() const
{
    return iPrivate != NULL;
}

const QString
QrCodeDecoder::Result::getText() const
{
    return iPrivate ? iPrivate->iText : QString();
}

const QList<QPointF>
QrCodeDecoder::Result::getPoints() const
{
    return iPrivate ? iPrivate->iPoints : QList<QPointF>();
}

const QString
QrCodeDecoder::Result::getFormatName() const
{
    return iPrivate ? iPrivate->iFormatName : QString();
}

// ==========================================================================
// QrCodeDecoder::Private
// ==========================================================================

class QrCodeDecoder::Private
{
public:
    Private();
    ~Private();

public:
    zbar::ImageScanner* iReader;
};

QrCodeDecoder::Private::Private() :
    iReader(new zbar::ImageScanner)
{
    iReader->set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
}

QrCodeDecoder::Private::~Private()
{
    delete iReader;
}

// ==========================================================================
// QrCodeDecoder
// ==========================================================================

QrCodeDecoder::QrCodeDecoder() :
    iPrivate(new Private)
{
    qRegisterMetaType<Result>();
}

QrCodeDecoder::~QrCodeDecoder()
{
    delete iPrivate;
}

QrCodeDecoder::Result
QrCodeDecoder::decode(
    const QImage aImage)
{
    try {
        zbar::Image img(zbar::QZBarImage(aImage).convert(zbar_fourcc('Y','8','0','0')));

        iPrivate->iReader->scan(img);

        const zbar::SymbolSet symbols(img.get_symbols());
        zbar::SymbolIterator sym = symbols.symbol_begin();
        if (sym != symbols.symbol_end()) {
            QList<QPointF> points;
            const zbar::Symbol symbol(*sym);
            zbar::Symbol::PointIterator it = symbol.point_begin();

            while (it != symbol.point_end()) {
                const zbar::Symbol::Point point(*it);

                points.append(QPointF(point.x, point.y));
                ++it;
            }

            return Result(QString::fromStdString(symbol.get_data()), points,
                QString::fromStdString(symbol.get_type_name()));
        }
    } catch (std::exception& x) {
        HWARN(x.what());
    }

    return Result();
}
