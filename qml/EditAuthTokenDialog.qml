import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

Dialog {
    id: thisDialog

    forwardNavigation: !qrCodeOnly
    backNavigation: !qrCodeOnly
    canAccept: generator.text !== ""

    property bool qrCodeOnly
    property bool canScan
    property alias acceptText: header.acceptText
    property alias dialogTitle: header.title
    property string issuer
    property int type: FoilAuth.DefaultType
    property int algorithm: FoilAuth.DefaultAlgorithm
    property alias label: labelField.text
    property alias secret: secretField.text
    property alias digits: digitsField.text
    property alias counter: counterField.text
    property alias timeshift: timeshiftField.text

    signal tokenAccepted(var dialog)

    onAccepted: tokenAccepted(thisDialog)

    HarbourQrCodeGenerator {
        id: generator

        ecLevel: FoilAuthSettings.qrCodeEcLevel
        text: FoilAuth.toUri(type, secret, label, issuer, digits, counter, timeshift, algorithm)
    }

    Item {
        id: fullScreenQrcodeContainer

        y: Math.round(((isPortrait ? Screen.height : Screen.width) - height)/2)
        width: qrcodeImage.width
        height: qrcodeImage.height
        anchors.horizontalCenter: thisDialog.horizontalCenter
        z: qrCodeOnly ? 1000 /* just above the dialog header overlay */ : 0
        opacity: qrCodeOnly ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { FadeAnimation { } }
    }

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: column.height
        interactive: opacity > 0
        Behavior on opacity { FadeAnimation { } }

        Column {
            id: column

            width: parent.width

            DialogHeader {
                id: header
                //: Dialog button
                //% "Save"
                acceptText: qsTrId("foilauth-edit_token-save")
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

                readonly property real columnWidth: width/columns

                TextField {
                    id: digitsField

                    width: parent.columnWidth
                    //: Text field label (number of password digits)
                    //% "Digits"
                    label: qsTrId("foilauth-token-digits-text")
                    //: Text field placeholder (number of password digits)
                    //% "Number of password digits"
                    placeholderText: qsTrId("foilauth-token-digits-placeholder")
                    text: FoilAuthDefaultDigits
                    validator: IntValidator { bottom: 1 }
                    inputMethodHints: Qt.ImhDigitsOnly
                    enabled: !qrCodeOnly

                    EnterKey.iconSource: "image://theme/icon-m-enter-next"
                    EnterKey.onClicked: timeshiftField.focus = true
                }

                TextField {
                    id: counterField

                    width: parent.columnWidth
                    //: Text field label (HOTP counter value)
                    //% "Counter value"
                    label: qsTrId("foilauth-token-counter-text")
                    placeholderText: label
                    text: FoilAuthDefaultCounter
                    validator: IntValidator {}
                    inputMethodHints: Qt.ImhDigitsOnly
                    enabled: !qrCodeOnly
                    visible: type === FoilAuth.TypeHOTP

                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: thisDialog.accept()
                }

                TextField {
                    id: timeshiftField

                    width: parent.columnWidth
                    //: Text field label (number of password digits)
                    //% "Time shift (seconds)"
                    label: qsTrId("foilauth-token-timeshift-text")
                    //: Text field placeholder (number of password digits)
                    //% "OTP time shift, in seconds"
                    placeholderText: qsTrId("foilauth-token-timeshift-placeholder")
                    text: FoilAuthDefaultTimeShift
                    validator: IntValidator {}
                    inputMethodHints: Qt.ImhDigitsOnly
                    enabled: !qrCodeOnly
                    visible: type === FoilAuth.TypeTOTP

                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: thisDialog.accept()
                }

                ComboBox {
                    id: algorithmComboBox

                    width: parent.columnWidth
                    //: Combo box label
                    //% "Digest algorithm"
                    label: qsTrId("foilauth-token-digest_algorithm-label")
                    menu: ContextMenu {
                        x: 0
                        width: algorithmComboBox.width
                        //: Menu item for the default digest algorithm
                        //% "%1 (default)"
                        MenuItem { text: qsTrId("foilauth-token-digest_algorithm-default").arg("SHA1") }
                        MenuItem { text: "SHA256" }
                        MenuItem { text: "SHA512" }
                    }
                    Component.onCompleted: currentIndex = algorithm
                    onCurrentIndexChanged: algorithm = currentIndex
                }

                ComboBox {
                    id: typeComboBox

                    width: parent.columnWidth
                    //: Combo box label
                    //% "Type"
                    label: qsTrId("foilauth-token-type-label")
                    menu: ContextMenu {
                        x: 0
                        width: typeComboBox.width
                        //: Menu item for time based token
                        //% "Time-based (TOTP)"
                        MenuItem { text: qsTrId("foilauth-token-type-totp") }
                        //: Menu item for counter based token
                        //% "Counter-based (HOTP)"
                        MenuItem { text: qsTrId("foilauth-token-type-hotp") }
                    }
                    Component.onCompleted: currentIndex = type
                    onCurrentIndexChanged: type = currentIndex
                }
            }
            VerticalPadding { }

            Button {
                id: scanButton

                anchors.horizontalCenter: parent.horizontalCenter
                //: Button label, opens QR code scan window
                //% "Scan QR code"
                text: qsTrId("foilauth-token-scan-button")
                visible: canScan && !secretField.text.length
                onClicked: pageStack.push(Qt.resolvedUrl("ScanPage.qml"), {
                    allowedOrientations: thisDialog.allowedOrientations
                })
            }

            VerticalPadding { visible: scanButton.visible }

            Item {
                id: flickableQrcodeContainer

                width: qrcodeImage.width
                height: qrcodeImage.height
                anchors.horizontalCenter: parent.horizontalCenter

                QRCodeImage {
                    id: qrcodeImage

                    anchors.horizontalCenter: parent.horizontalCenter
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

    transitions: [
        Transition {
            to: "qrcode"

            NumberAnimation {
                target: qrcodeImage
                property: "y"
                from: flickableQrcodeContainer.mapToItem(fullScreenQrcodeContainer, 0, 0).y
                to: 0
                duration: 200
            }
        },
        Transition {
            from: "qrcode"
            to: "normal"

            NumberAnimation {
                target: qrcodeImage
                property: "y"
                from: fullScreenQrcodeContainer.mapToItem(flickableQrcodeContainer, 0, 0).y
                to: 0
                duration: 200
            }
        }
    ]
}
