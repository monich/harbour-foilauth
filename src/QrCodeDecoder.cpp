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

#include "QrCodeDecoder.h"
#include "QrCodeImageSource.h"

#include "HarbourDebug.h"

#include <QAtomicInt>

#include <zxing/Binarizer.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/DecodeHints.h>
#include <zxing/Reader.h>

#include <zxing/qrcode/QRCodeReader.h>

#include <zxing/common/GlobalHistogramBinarizer.h>

// ==========================================================================
// QrCodeDecoder::Result::Private
// ==========================================================================

class QrCodeDecoder::Result::Private {
public:
    Private(QString aText, QList<QPointF> aPoints, zxing::BarcodeFormat::Value aFormat);

public:
    QAtomicInt iRef;
    QString iText;
    QList<QPointF> iPoints;
    zxing::BarcodeFormat::Value iFormat;
    QString iFormatName;
};

QrCodeDecoder::Result::Private::Private(QString aText, QList<QPointF> aPoints,
    zxing::BarcodeFormat::Value aFormat) :
    iRef(1), iText(aText), iPoints(aPoints), iFormat(aFormat),
    iFormatName(QLatin1String(zxing::BarcodeFormat::barcodeFormatNames[aFormat]))
{
}

// ==========================================================================
// QrCodeDecoder::Result
// ==========================================================================

QrCodeDecoder::Result::Result(QString aText, QList<QPointF> aPoints,
    zxing::BarcodeFormat aFormat) :
    iPrivate(new Private(aText, aPoints, aFormat))
{
}

QrCodeDecoder::Result::Result(const Result& aResult) :
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

QrCodeDecoder::Result& QrCodeDecoder::Result::operator = (const Result& aResult)
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

bool QrCodeDecoder::Result::isValid() const
{
    return iPrivate != NULL;
}

QString QrCodeDecoder::Result::getText() const
{
    return iPrivate ? iPrivate->iText : QString();
}

QList<QPointF> QrCodeDecoder::Result::getPoints() const
{
    return iPrivate ? iPrivate->iPoints : QList<QPointF>();
}

zxing::BarcodeFormat::Value QrCodeDecoder::Result::getFormat() const
{
    return iPrivate ? iPrivate->iFormat : zxing::BarcodeFormat::NONE;
}

QString QrCodeDecoder::Result::getFormatName() const
{
    return iPrivate ? iPrivate->iFormatName : QString();
}

// ==========================================================================
// Decoder::Private
// ==========================================================================

class QrCodeDecoder::Private {
public:
    Private();

    zxing::Ref<zxing::Result> decode(zxing::Ref<zxing::LuminanceSource> aSource);

public:
    zxing::Ref<zxing::Reader> iReader;
    zxing::DecodeHints iHints;
};

QrCodeDecoder::Private::Private() :
    iReader(new zxing::qrcode::QRCodeReader),
    iHints(zxing::DecodeHints::DEFAULT_HINT)
{
}

zxing::Ref<zxing::Result> QrCodeDecoder::Private::decode(zxing::Ref<zxing::LuminanceSource> aSource)
{
    zxing::Ref<zxing::Binarizer> binarizer(new zxing::GlobalHistogramBinarizer(aSource));
    zxing::Ref<zxing::BinaryBitmap> bitmap(new zxing::BinaryBitmap(binarizer));
    return iReader->decode(bitmap, iHints);
}

// ==========================================================================
// QrCodeDecoder
// ==========================================================================

QrCodeDecoder::QrCodeDecoder() : iPrivate(new Private)
{
    qRegisterMetaType<Result>();
}

QrCodeDecoder::~QrCodeDecoder()
{
    delete iPrivate;
}

QrCodeDecoder::Result QrCodeDecoder::decode(QImage aImage)
{
    zxing::Ref<zxing::LuminanceSource> source(new QrCodeImageSource(aImage));
    return decode(source);
}

QrCodeDecoder::Result QrCodeDecoder::decode(zxing::Ref<zxing::LuminanceSource> aSource)
{
    try {
        zxing::Ref<zxing::Result> result(iPrivate->decode(aSource));

        QList<QPointF> points;
        zxing::ArrayRef<zxing::Ref<zxing::ResultPoint> > found(result->getResultPoints());
        for (int i = 0; i < found->size(); i++) {
            const zxing::ResultPoint& point(*(found[i]));
            points.append(QPointF(point.getX(), point.getY()));
        }

        return Result(result->getText()->getText().c_str(), points,
            result->getBarcodeFormat());
    } catch (zxing::Exception& except) {
        HDEBUG("Exception:" << except.what());
        return Result();
    }
}
