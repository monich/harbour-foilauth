import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

import "harbour"

Dialog {
    id: thisDialog

    forwardNavigation: !_qrCodeFullScreen
    backNavigation: !_qrCodeFullScreen
    canAccept: _uri !== ""

    property bool canImportFromClipboard: true
    property bool showQrCode: !canImportFromClipboard
    property alias acceptText: header.acceptText
    property alias dialogTitle: header.title
    property string issuer
    property alias type: typeComboBox.currentIndex
    property alias algorithm: algorithmComboBox.currentIndex
    property alias label: labelField.text
    property alias secret: secretField.text
    property alias digits: digitsField.text
    property alias counter: counterField.text
    property alias timeshift: timeshiftField.text

    property bool _qrCodeFullScreen
    property string _uri: FoilAuth.toUri(type, secret, label, issuer, digits, counter, timeshift, algorithm)

    signal tokenAccepted(var dialog)

    onAccepted: tokenAccepted(thisDialog)

    HarbourQrCodeGenerator {
        id: generator

        text: showQrCode ? _uri : ""
        ecLevel: FoilAuthSettings.qrCodeEcLevel
    }

    Item {
        id: fullScreenQrcodeContainer

        y: Math.round(((isPortrait ? Screen.height : Screen.width) - height)/2)
        width: qrcodeImage.width
        height: qrcodeImage.height
        anchors.horizontalCenter: thisDialog.horizontalCenter
        z: _qrCodeFullScreen ? 1000 /* just above the dialog header overlay */ : 0
        opacity: _qrCodeFullScreen ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { FadeAnimation { } }
    }

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: column.height
        interactive: opacity > 0
        Behavior on opacity { FadeAnimation { } }


        PullDownMenu {
            id: pullDownMenu

            visible: canImportFromClipboard

            MenuItem {
                readonly property var clipboardToken: canImportFromClipboard ?
                    FoilAuth.parseUri(Clipboard.text) : { "valid": false }

                enabled: !!clipboardToken.valid
                text: "Import from clipboard"
                onClicked: {
                    var token = clipboardToken
                    Clipboard.text = ""
                    typeComboBox.currentIndex = token.type
                    algorithmComboBox.currentIndex = token.algorithm
                    thisDialog.label = token.label
                    thisDialog.issuer = token.issuer
                    thisDialog.secret = token.secret
                    thisDialog.digits = token.digits
                    thisDialog.counter = token.counter
                    thisDialog.timeshift = token.timeshift
                }
            }
        }

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
                enabled: !_qrCodeFullScreen

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
              inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhSensitiveData
              enabled: !_qrCodeFullScreen

              EnterKey.enabled: text.length > 10
              EnterKey.iconSource: "image://theme/icon-m-enter-next"
              EnterKey.onClicked: digitsField.focus = true

              // Don't select the secret
              onSelectedTextChanged: {
                  if (selectedText !== "") {
                      deselect()
                  }
              }
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
                    text: FoilAuth.DefaultDigits
                    validator: IntValidator {
                        bottom: FoilAuth.MinDigits
                        top: FoilAuth.MaxDigits
                    }
                    inputMethodHints: Qt.ImhDigitsOnly
                    enabled: !_qrCodeFullScreen

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
                    text: FoilAuth.DefaultCounter
                    validator: IntValidator {}
                    inputMethodHints: Qt.ImhDigitsOnly
                    enabled: !_qrCodeFullScreen
                    visible: type === FoilAuth.TypeHOTP

                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: thisDialog.accept()
                }

                TextField {
                    id: timeshiftField

                    width: parent.columnWidth
                    textRightMargin: Theme.paddingLarge/2 + timeshiftEditButton.width + textMargin
                    //: Text field label (number of password digits)
                    //% "Time shift (seconds)"
                    label: qsTrId("foilauth-token-timeshift-text")
                    //: Text field placeholder (number of password digits)
                    //% "OTP time shift, in seconds"
                    placeholderText: qsTrId("foilauth-token-timeshift-placeholder")
                    text: FoilAuth.DefaultTimeShift
                    validator: IntValidator {}
                    inputMethodHints: Qt.ImhDigitsOnly
                    enabled: !_qrCodeFullScreen
                    visible: type === FoilAuth.TypeTOTP || type === FoilAuth.TypeSteam

                    EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                    EnterKey.onClicked: thisDialog.accept()

                    MouseArea {
                        id: timeshiftEditButton

                        parent: timeshiftField
                        x: parent.width - width - parent.textMargin
                        width: textEllipsis.implicitWidth + Theme.paddingLarge/2
                        height: parent.height - Theme.paddingLarge

                        Text {
                            id: textEllipsis

                            anchors {
                                top: parent.top
                                topMargin: timeshiftField.textTopMargin
                                horizontalCenter: parent.horizontalCenter
                            }
                            font.pixelSize: Theme.fontSizeMedium
                            text: "…"
                            textFormat: Text.PlainText
                            color: parent.pressed && parent.containsMouse ? Theme.highlightColor : Theme.primaryColor
                        }

                        onClicked: {
                            var editor = pageStack.push(Qt.resolvedUrl("TimeShiftDialog.qml"), {
                                allowedOrientations: thisDialog.allowedOrientations,
                                timeshift: thisDialog.timeshift / 60
                            })
                            editor.accepted.connect(function() {
                                thisDialog.timeshift = editor.timeshift * 60
                            })
                        }
                    }
                }

                ComboBox {
                    id: algorithmComboBox

                    width: parent.columnWidth
                    //: Combo box label
                    //% "Digest algorithm"
                    label: qsTrId("foilauth-token-digest_algorithm-label")
                    currentIndex: FoilAuth.DefaultAlgorithm
                    menu: ContextMenu {
                        x: 0
                        width: algorithmComboBox.width
                        //: Menu item for the default digest algorithm
                        //% "%1 (default)"
                        MenuItem { text: qsTrId("foilauth-token-digest_algorithm-default").arg("SHA1") }
                        MenuItem { text: "SHA256" }
                        MenuItem { text: "SHA512" }
                        onActivated: thisDialog.algorithm = index
                    }
                }

                ComboBox {
                    id: typeComboBox

                    width: parent.columnWidth
                    //: Combo box label
                    //% "Type"
                    label: qsTrId("foilauth-token-type-label")
                    currentIndex: FoilAuth.DefaultType
                    menu: ContextMenu {
                        x: 0
                        width: typeComboBox.width
                        //: Menu item for time based token
                        //% "Time-based (TOTP)"
                        MenuItem { text: qsTrId("foilauth-token-type-totp") }
                        //: Menu item for counter based token
                        //% "Counter-based (HOTP)"
                        MenuItem { text: qsTrId("foilauth-token-type-hotp") }
                        //: Menu item for time based token
                        //% "Steam"
                        MenuItem { text: qsTrId("foilauth-token-type-steam") }
                        onActivated: thisDialog.type = index
                    }
                }
            }

            VerticalPadding { }

            Item {
                id: flickableQrcodeContainer

                width: qrcodeImage.width
                height: qrcodeImage.height
                visible: generator.qrcode !== ""
                anchors.horizontalCenter: parent.horizontalCenter

                QRCodeImage {
                    id: qrcodeImage

                    anchors.horizontalCenter: parent.horizontalCenter
                    qrcode: generator.qrcode

                    MouseArea {
                        id: qrcodeMouseArea

                        enabled: !_qrCodeFullScreen
                        anchors.fill: parent
                        onPressAndHold: ;
                        onClicked: {
                            _qrCodeFullScreen = true
                            flickable.focus = true
                        }
                    }

                    layer.effect: HarbourPressEffect { source: qrcodeImage }
                    layer.enabled: (qrcodeMouseArea.pressed && qrcodeMouseArea.containsMouse) ||
                        (fullScreenQrcodeMouseArea.pressed && fullScreenQrcodeMouseArea.containsMouse)
                }
            }

            VerticalPadding { visible: flickableQrcodeContainer.visible }
        }

        VerticalScrollDecorator { }
    }

    MouseArea {
        id: fullScreenQrcodeMouseArea

        enabled: _qrCodeFullScreen
        anchors.fill: parent
        onPressAndHold: ;
        onClicked: _qrCodeFullScreen = false
    }

    states: [
        State {
            name: "qrcode"
            when: _qrCodeFullScreen

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
            when: !_qrCodeFullScreen

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
