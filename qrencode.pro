TEMPLATE = lib
CONFIG += static
TARGET = qrencode
QT-= gui

SRC_DIR = $${_PRO_FILE_PWD_}/libqrencode

MAJOR_VERSION = 4
MINOR_VERSION = 1
MICRO_VERSION = 1

DEFINES += \
    STATIC_IN_RELEASE=static \
    MAJOR_VERSION=$${MAJOR_VERSION} \
    MINOR_VERSION=$${MINOR_VERSION} \
    MICRO_VERSION=$${MICRO_VERSION} \
    VERSION=\\\"$${MAJOR_VERSION}.$${MINOR_VERSION}.$${MICRO_VERSION}\\\"

SOURCES += \
    $${SRC_DIR}/bitstream.c \
    $${SRC_DIR}/mask.c \
    $${SRC_DIR}/mmask.c \
    $${SRC_DIR}/mqrspec.c \
    $${SRC_DIR}/rsecc.c \
    $${SRC_DIR}/split.c \
    $${SRC_DIR}/qrencode.c \
    $${SRC_DIR}/qrinput.c \
    $${SRC_DIR}/qrspec.c
