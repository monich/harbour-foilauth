TEMPLATE = lib
CONFIG += static
TARGET = zxing
QT-= gui

ZXING_DIR = $${_PRO_FILE_PWD_}/zxing
SRC_DIR = $${ZXING_DIR}

INCLUDEPATH += \
  $${ZXING_DIR}

DEFINES +=

HEADERS += \
    $${SRC_DIR}/BarcodeFormat.h \
    $${SRC_DIR}/Binarizer.h \
    $${SRC_DIR}/BinaryBitmap.h \
    $${SRC_DIR}/ChecksumException.h \
    $${SRC_DIR}/DecodeHints.h \
    $${SRC_DIR}/Exception.h \
    $${SRC_DIR}/FormatException.h \
    $${SRC_DIR}/InvertedLuminanceSource.h \
    $${SRC_DIR}/LuminanceSource.h \
    $${SRC_DIR}/Reader.h \
    $${SRC_DIR}/Result.h \
    $${SRC_DIR}/ResultPoint.h

SOURCES += \
    $${SRC_DIR}/BarcodeFormat.cpp \
    $${SRC_DIR}/Binarizer.cpp \
    $${SRC_DIR}/BinaryBitmap.cpp \
    $${SRC_DIR}/DecodeHints.cpp \
    $${SRC_DIR}/ChecksumException.cpp \
    $${SRC_DIR}/Exception.cpp \
    $${SRC_DIR}/FormatException.cpp \
    $${SRC_DIR}/InvertedLuminanceSource.cpp \
    $${SRC_DIR}/LuminanceSource.cpp \
    $${SRC_DIR}/Reader.cpp \
    $${SRC_DIR}/Result.cpp \
    $${SRC_DIR}/ResultPoint.cpp

HEADERS += \
    $${SRC_DIR}/common/BitArray.h \
    $${SRC_DIR}/common/BitMatrix.h \
    $${SRC_DIR}/common/BitSource.h \
    $${SRC_DIR}/common/CharacterSetECI.h \
    $${SRC_DIR}/common/Counted.h \
    $${SRC_DIR}/common/DecoderResult.h \
    $${SRC_DIR}/common/DetectorResult.h \
    $${SRC_DIR}/common/GlobalHistogramBinarizer.h \
    $${SRC_DIR}/common/GridSampler.h \
    $${SRC_DIR}/common/IllegalArgumentException.h \
    $${SRC_DIR}/common/PerspectiveTransform.h \
    $${SRC_DIR}/common/Str.h \
    $${SRC_DIR}/common/StringUtils.h \
    $${SRC_DIR}/common/Types.h \
    $${SRC_DIR}/common/reedsolomon/GenericGF.h \
    $${SRC_DIR}/common/reedsolomon/GenericGFPoly.h \
    $${SRC_DIR}/common/reedsolomon/ReedSolomonDecoder.h \
    $${SRC_DIR}/common/reedsolomon/ReedSolomonException.h

SOURCES += \
    $${SRC_DIR}/common/BitArray.cpp \
    $${SRC_DIR}/common/BitMatrix.cpp \
    $${SRC_DIR}/common/BitSource.cpp \
    $${SRC_DIR}/common/CharacterSetECI.cpp \
    $${SRC_DIR}/common/DecoderResult.cpp \
    $${SRC_DIR}/common/DetectorResult.cpp \
    $${SRC_DIR}/common/GlobalHistogramBinarizer.cpp \
    $${SRC_DIR}/common/GridSampler.cpp \
    $${SRC_DIR}/common/IllegalArgumentException.cpp \
    $${SRC_DIR}/common/PerspectiveTransform.cpp \
    $${SRC_DIR}/common/Str.cpp \
    $${SRC_DIR}/common/StringUtils.cpp \
    $${SRC_DIR}/common/reedsolomon/GenericGF.cpp \
    $${SRC_DIR}/common/reedsolomon/GenericGFPoly.cpp \
    $${SRC_DIR}/common/reedsolomon/ReedSolomonDecoder.cpp \
    $${SRC_DIR}/common/reedsolomon/ReedSolomonException.cpp

HEADERS += \
    $${SRC_DIR}/qrcode/ErrorCorrectionLevel.h \
    $${SRC_DIR}/qrcode/FormatInformation.h \
    $${SRC_DIR}/qrcode/QRCodeReader.h \
    $${SRC_DIR}/qrcode/Version.h \
    $${SRC_DIR}/qrcode/decoder/BitMatrixParser.h \
    $${SRC_DIR}/qrcode/decoder/DataBlock.h \
    $${SRC_DIR}/qrcode/decoder/DataMask.h \
    $${SRC_DIR}/qrcode/decoder/DecodedBitStreamParser.h \
    $${SRC_DIR}/qrcode/decoder/Decoder.h \
    $${SRC_DIR}/qrcode/decoder/Mode.h \
    $${SRC_DIR}/qrcode/detector/AlignmentPatternFinder.h \
    $${SRC_DIR}/qrcode/detector/AlignmentPattern.h \
    $${SRC_DIR}/qrcode/detector/FinderPatternFinder.h \
    $${SRC_DIR}/qrcode/detector/FinderPattern.h \
    $${SRC_DIR}/qrcode/detector/FinderPatternInfo.h \
    $${SRC_DIR}/qrcode/detector/Detector.h

SOURCES += \
    $${SRC_DIR}/qrcode/QRCodeReader.cpp \
    $${SRC_DIR}/qrcode/QRErrorCorrectionLevel.cpp \
    $${SRC_DIR}/qrcode/QRFormatInformation.cpp \
    $${SRC_DIR}/qrcode/QRVersion.cpp \
    $${SRC_DIR}/qrcode/decoder/QRBitMatrixParser.cpp \
    $${SRC_DIR}/qrcode/decoder/QRDecodedBitStreamParser.cpp \
    $${SRC_DIR}/qrcode/decoder/QRDataBlock.cpp \
    $${SRC_DIR}/qrcode/decoder/QRDataMask.cpp \
    $${SRC_DIR}/qrcode/decoder/QRDecoder.cpp \
    $${SRC_DIR}/qrcode/decoder/QRMode.cpp \
    $${SRC_DIR}/qrcode/detector/QRAlignmentPattern.cpp \
    $${SRC_DIR}/qrcode/detector/QRAlignmentPatternFinder.cpp \
    $${SRC_DIR}/qrcode/detector/QRDetector.cpp \
    $${SRC_DIR}/qrcode/detector/QRFinderPattern.cpp \
    $${SRC_DIR}/qrcode/detector/QRFinderPatternFinder.cpp \
    $${SRC_DIR}/qrcode/detector/QRFinderPatternInfo.cpp
