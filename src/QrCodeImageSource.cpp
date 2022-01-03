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

#include "QrCodeImageSource.h"

class QrCodeImageSource::Private {
public:
    static QImage gray(const QImage& aImage);
    static bool isGray(const QImage& aImage);
    static const QVector<QRgb> grayColorTable();
    static const QVector<QRgb> gGrayColorTable;
};

const QVector<QRgb> QrCodeImageSource::Private::gGrayColorTable
    (QrCodeImageSource::Private::grayColorTable());

const QVector<QRgb> QrCodeImageSource::Private::grayColorTable()
{
    QVector<QRgb> colors;
    colors.reserve(256);
    for (int i = 0; i < 256; i++) {
        colors.append(qRgb(i, i, i));
    }
    return colors;
}

bool QrCodeImageSource::Private::isGray(const QImage& aImage)
{
    if (aImage.format() == QImage::Format_Indexed8) {
        const int n = gGrayColorTable.count();
        if (aImage.colorCount() == n) {
            const QVector<QRgb> ct = aImage.colorTable();
            const QRgb* data1 = ct.constData();
            const QRgb* data2 = gGrayColorTable.constData();
            // Most of the time pointers would match
            if (data1 == data2 || !memcmp(data1, data2, sizeof(QRgb)*n)) {
                return true;
            }
        }
    }
    return false;
}

QImage QrCodeImageSource::Private::gray(const QImage& aImage)
{
    if (isGray(aImage) || aImage.isNull()) {
        return aImage;
    } else if (aImage.depth() != 32) {
        return gray(aImage.convertToFormat(QImage::Format_RGB32));
    } else {
        const int w = aImage.width();
        const int h =  aImage.height();
        QImage gray(w, h, QImage::Format_Indexed8);
        gray.setColorTable(gGrayColorTable);

        const uchar* idata = aImage.constBits();
        const uint istride = aImage.bytesPerLine();
        uchar* odata = gray.bits();
        const uint ostride = gray.bytesPerLine();

        for (int y = 0; y < h; y++, idata += istride, odata += ostride) {
            uchar* dest = odata;
            const QRgb* src = (const QRgb*)idata;
            for (int x = 0; x < w; x++) {
                const QRgb rgb = *src++;
                // *dest++ = qGray(rgb);
                // This is significantly faster than gGray() but is
                // just as good for our purposes:
                *dest++ = (uchar)((((rgb & 0x00ff0000) >> 16) +
                    ((rgb & 0x0000ff00) >> 8) +
                    (rgb & 0xff))/3);
            }
        }
        return gray;
    }
}

QrCodeImageSource::QrCodeImageSource(QImage aImage) :
    zxing::LuminanceSource(aImage.width(), aImage.height()),
    iImage(Private::gray(aImage))
{
}

QrCodeImageSource::~QrCodeImageSource()
{
}

zxing::ArrayRef<zxing::byte> QrCodeImageSource::getRow(int aY, zxing::ArrayRef<zxing::byte> aRow) const
{
    const int width = getWidth();
    if (aRow->size() != width) {
        aRow.reset(zxing::ArrayRef<zxing::byte>(width));
    }
    memcpy(&aRow[0], getGrayRow(aY), width);
    return aRow;
}

zxing::ArrayRef<zxing::byte> QrCodeImageSource::getMatrix() const
{
    const int width = getWidth();
    const int height =  getHeight();
    zxing::ArrayRef<zxing::byte> matrix(width*height);
    zxing::byte* m = &matrix[0];

    for (int y = 0; y < height; y++) {
        memcpy(m, getGrayRow(y), width);
        m += width;
    }

    return matrix;
}

const zxing::byte* QrCodeImageSource::getGrayRow(int aY) const
{
    return iImage.constBits() + aY * iImage.bytesPerLine();
}

QImage QrCodeImageSource::grayscaleImage() const
{
    return iImage;
}
