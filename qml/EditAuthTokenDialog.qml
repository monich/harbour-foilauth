import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

Dialog {
    id: thisDialog

    allowedOrientations: appAllowedOrientations
    forwardNavigation: !qrCodeOnly
    backNavigation: !qrCodeOnly
    canAccept: generator.text !== ""

    property bool qrCodeOnly
    property bool canScan
    property alias acceptText: header.acceptText
    property alias dialogTitle: header.title
    property string issuer
    property alias label: labelField.text
    property alias secret: secretField.text
    property alias digits: digitsField.text
    property alias timeShift: timeShiftField.text

    HarbourQrCodeGenerator {
        id: generator

        text: FoilAuth.toUri(secretField.text, labelField.text, thisDialog.issuer,
            digitsField.text, timeShiftField.text)
    }

    Item {
        id: fullScreenQrcodeContainer

        y: Math.round(((isPortrait ? Screen.height : Screen.width) - height)/2)
        width: qrcodeImage.width
        height: qrcodeImage.height
        anchors.horizontalCenter: thisDialog.horizontalCenter
        visible: qrCodeOnly
    }

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: column.height
        visible: opacity > 0
        Behavior on opacity { FadeAnimation { } }

        Column {
            id: column

            width: parent.width

            DialogHeader {
                id: header
            }

            TextField {
                id: labelField

                width: parent.width
                //: Text field label (OTP label)
                //% "Label"
                label: qsTrId("foilauth-token-label-text")
                //: Text field placeholder (OTP label)
                //% "OTP label"
                placeholderText: qsTrId("foilauth-token-label-placeholder")
                enabled: !qrCodeOnly

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
              enabled: !qrCodeOnly

              EnterKey.enabled: text.length > 10
              EnterKey.iconSource: "image://theme/icon-m-enter-next"
              EnterKey.onClicked: digitsField.focus = true
            }

            Grid {
                columns: isLandscape ? 2 : 1
                width: parent.width

                TextField {
                    id: digitsField

                    width: parent.width/parent.columns
                    //: Text field label (number of password digits)
                    //% "Digits"
                    label: qsTrId("foilauth-token-digits-text")
                    //: Text field placeholder (number of password digits)
                    //% "Number of password digits"
                    placeholderText: qsTrId("foilauth-token-digits-placeholder")
                    text: FoilAuthDefaultDigits
                    validator: IntValidator { bottom: 1 }
                    enabled: !qrCodeOnly

                    EnterKey.iconSource: "image://theme/icon-m-enter-next"
                    EnterKey.onClicked: timeShiftField.focus = true
                }

                TextField {
                    id: timeShiftField

                    width: parent.width/parent.columns
                    //: Text field label (number of password digits)
                    //% "Time shift (seconds)"
                    label: qsTrId("foilauth-token-timeshift-text")
                    //: Text field placeholder (number of password digits)
                    //% "OTP time shift, in seconds"
                    placeholderText: qsTrId("foilauth-token-timeshift-placeholder")
                    text: FoilAuthDefaultTimeShift
                    validator: IntValidator {}
                    enabled: !qrCodeOnly

                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: thisDialog.accept()
                }
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

            Item {
                id: flickableQrcodeContainer

                width: qrcodeImage.width
                height: qrcodeImage.height
                anchors.horizontalCenter: parent.horizontalCenter

                QRCodeImage {
                    id: qrcodeImage

                    anchors.centerIn: parent
                    qrcode: generator.qrcode

                    MouseArea {
                        enabled: !qrCodeOnly
                        anchors.fill: parent
                        onClicked: {
                            qrCodeOnly = true
                            flickable.focus = true
                        }
                    }
                }
            }

            VerticalPadding { }
        }

        VerticalScrollDecorator { }
    }

    MouseArea {
        enabled: qrCodeOnly
        anchors.fill: parent
        onClicked: qrCodeOnly = false
    }

    states: [
        State {
            name: "qrcode"
            when: qrCodeOnly
            ParentChange {
                target: qrcodeImage
                parent: fullScreenQrcodeContainer
            }
            PropertyChanges {
                target: flickable
                opacity: 0
            }
        },
        State {
            name: "normal"
            when: !qrCodeOnly
            PropertyChanges {
                target: flickable
                opacity: 1
            }
            ParentChange {
                target: qrcodeImage
                parent: flickableQrcodeContainer
            }
        }
    ]
}
