import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

Dialog {
    id: dialog

    allowedOrientations: appAllowedOrientations
    canAccept: generator.text !== ""

    property bool canScan
    property alias acceptText: header.acceptText
    property string issuer
    property alias label: labelField.text
    property alias secret: secretField.text
    property alias digits: digitsField.text
    property alias timeShift: timeShiftField.text

    DialogHeader {
        id: header

        spacing: 0
    }

    HarbourQrCodeGenerator {
        id: generator

        text: FoilAuth.toUri(secretField.text, labelField.text, dialog.issuer,
            digitsField.text, timeShiftField.text)
    }

    SilicaFlickable {
        anchors {
            top: header.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        contentHeight: column.height
        clip: true

        Column {
            id: column

            width: parent.width

            VerticalPadding { }

            TextField {
                id: labelField

                width: parent.width
                //: Text field label (OTP label)
                //% "Label"
                label: qsTrId("foilauth-token-label-text")
                //: Text field placeholder (OTP label)
                //% "OTP label"
                placeholderText: qsTrId("foilauth-token-label-placeholder")

                EnterKey.enabled: text.length > 0
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: secretField.focus = true
            }

            TextField {
              id: secretField

              width: parent.width
              //: Text field label (OTP secret)
              //% "Secret"
              label: qsTrId("foilauth-token-secret-text")
              //: Text field placeholder (OTP secret)
              //% "Secret OTP key"
              placeholderText: qsTrId("foilauth-token-secret-placeholder")
              errorHighlight: !FoilAuth.isValidBase32(text)

              EnterKey.enabled: text.length > 10
              EnterKey.iconSource: "image://theme/icon-m-enter-next"
              EnterKey.onClicked: digitsField.focus = true
            }

            TextField {
                id: digitsField

                width: parent.width
                //: Text field label (number of password digits)
                //% "Digits"
                label: qsTrId("foilauth-token-digits-text")
                //: Text field placeholder (number of password digits)
                //% "Number of password digits"
                placeholderText: qsTrId("foilauth-token-digits-placeholder")
                text: FoilAuthDefaultDigits
                validator: IntValidator { bottom: 1 }

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: timeShiftField.focus = true
            }

            TextField {
                id: timeShiftField

                width: parent.width
                //: Text field label (number of password digits)
                //% "Time shift (seconds)"
                label: qsTrId("foilauth-token-timeshift-text")
                //: Text field placeholder (number of password digits)
                //% "OTP time shift, in seconds"
                placeholderText: qsTrId("foilauth-token-timeshift-placeholder")
                text: FoilAuthDefaultTimeShift
                validator: IntValidator {}

                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: dialog.accept()
            }

            VerticalPadding { }

            Button {
                id: scanButton

                anchors.horizontalCenter: parent.horizontalCenter
                //: Button label, opens QR code scan window
                //% "Scan QR code"
                text: qsTrId("foilauth-token-scan-button")
                visible: canScan && !secretField.text
                onClicked: {
                    var scanPage = pageStack.push(Qt.resolvedUrl("ScanPage.qml"))
                    if (scanPage) {
                        scanPage.scanCompleted.connect(function(token) {
                            if (token.valid) {
                                labelField.text = token.label
                                secretField.text = token.secret
                                digitsField.text = token.digits
                                timeShiftField.text = token.timeshift
                            }
                        })
                    }
                }
            }

            VerticalPadding { visible: scanButton.visible }

            Rectangle {
                color: "white"
                radius: Theme.paddingMedium
                x: Math.floor((parent.width - width)/2)
                width: qrcodeImage.width + 2 * Theme.horizontalPageMargin
                height: qrcodeImage.height + 2 * Theme.horizontalPageMargin
                visible: generator.qrcode !== ""

                Image {
                    id: qrcodeImage

                    asynchronous: true
                    anchors.centerIn: parent
                    source: generator.qrcode ? "image://qrcode/" + generator.qrcode : ""
                    readonly property int maxDisplaySize: Math.min(Screen.width, Screen.height) - Theme.itemSizeLarge - 4 * Theme.horizontalPageMargin
                    readonly property int maxSourceSize: Math.max(sourceSize.width, sourceSize.height)
                    readonly property int n: Math.floor(maxDisplaySize/maxSourceSize)
                    width: sourceSize.width * n
                    height: sourceSize.height * n
                    smooth: false
                }
            }

            VerticalPadding { }
        }

        VerticalScrollDecorator { }
    }
}
