import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

import "harbour"

Item {
    id: view

    property Page mainPage
    property alias title: title.text
    readonly property int minPassphraseLen: 8
    readonly property var foilModel: FoilAuthModel
    readonly property bool generating: foilModel.foilState === FoilAuthModel.FoilGeneratingKey
    readonly property bool landscapeLayout: appLandscapeMode && Screen.sizeCategory < Screen.Large

    function generateKey() {
        var confirm = pageStack.push(Qt.resolvedUrl("ConfirmPasswordPage.qml"), {
            password: inputField.text
        })
        confirm.passwordConfirmed.connect(function() {
            confirm.backNavigation = false
            foilModel.generateKey(keySize.value, inputField.text)
            pageStack.pop(mainPage)
        })
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height
        y: (parent.height > height) ? Math.floor((parent.height - height)/2) : (parent.height - height)

        InfoLabel {
            id: title

            height: implicitHeight
            //: Label text
            //% "You need to generate the key and select the password before you can encrypt your authentication tokens"
            text: qsTrId("foilauth-generate-label-key_needed")
        }

        ComboBox {
            id: keySize

            //: Combo box label
            //% "Key size"
            label: qsTrId("foilauth-generate-label-key_size")
            enabled: !generating
            width: parent.width
            anchors {
                top: title.bottom
                topMargin: Theme.paddingLarge
            }
            menu: ContextMenu {
                MenuItem { text: "1024" }
                MenuItem { text: "1500" }
                MenuItem { text: "2048" }
            }
            Component.onCompleted: currentIndex = 1 // default
        }

        HarbourPasswordInputField {
            id: inputField

            anchors {
                left: parent.left
                top: keySize.bottom
                topMargin: Theme.paddingLarge
            }
            label: text.length < minPassphraseLen ?
                //: Password field label
                //% "Type at least %0 character(s)"
                qsTrId("foilauth-generate-label-minimum_length",minPassphraseLen).arg(minPassphraseLen) :
                placeholderText
            enabled: !generating
            EnterKey.onClicked: generateKey()
        }

        Button {
            id: button

            text: generating ?
                //: Button label
                //% "Generating..."
                qsTrId("foilauth-generate-button-generating_key") :
                //: Button label
                //% "Generate key"
                qsTrId("foilauth-generate-button-generate_key")
            enabled: inputField.text.length >= minPassphraseLen && !generating
            onClicked: generateKey()
        }
    }

    states: [
        State {
            name: "portrait"
            when: !landscapeLayout
            changes: [
                AnchorChanges {
                    target: inputField
                    anchors.right: panel.right
                },
                PropertyChanges {
                    target: inputField
                    anchors.rightMargin: 0
                },
                AnchorChanges {
                    target: button
                    anchors {
                        top: inputField.bottom
                        horizontalCenter: parent.horizontalCenter
                    }
                },
                PropertyChanges {
                    target: button
                    anchors {
                        topMargin: Theme.paddingLarge
                        rightMargin: 0
                    }
                }
            ]
        },
        State {
            name: "landscape"
            when: landscapeLayout
            changes: [
                AnchorChanges {
                    target: inputField
                    anchors.right: button.left
                },
                PropertyChanges {
                    target: inputField
                    anchors.rightMargin: Theme.horizontalPageMargin
                },
                AnchorChanges {
                    target: button
                    anchors {
                        top: keySize.bottom
                        right: panel.right
                        horizontalCenter: undefined
                    }
                },
                PropertyChanges {
                    target: button
                    anchors {
                        topMargin: Theme.paddingLarge
                        rightMargin: Theme.horizontalPageMargin
                    }
                }
            ]
        }
    ]
}
