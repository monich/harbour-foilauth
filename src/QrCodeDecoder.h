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

#ifndef QRCODE_DECODER_H
#define QRCODE_DECODER_H

#include <QList>
#include <QImage>
#include <QPoint>
#include <QString>
#include <QMetaType>

class QrCodeDecoder
{
    Q_DISABLE_COPY(QrCodeDecoder)

public:
    class Result;

    QrCodeDecoder();
    ~QrCodeDecoder();

    Result decode(const QImage);

private:
    class Private;
    Private* iPrivate;
};

class QrCodeDecoder::Result
{
friend class QrCodeDecoder;

public:
    Result(const QString, const QList<QPointF>, const QString);
    Result(const Result&);
    Result();
    ~Result();

    Result& operator=(const Result&);

    bool isValid() const;
    const QString getText() const;
    const QList<QPointF> getPoints() const;
    const QString getFormatName() const;

private:
    class Private;
    Private* iPrivate;
};

Q_DECLARE_METATYPE(QrCodeDecoder::Result)

#endif // QRCODE_DECODER_H
