import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

import "harbour"

Item {
    id: view

    property Page parentPage
    property Page mainPage
    property var foilModel
    property alias prompt: promptLabel.text

    readonly property int minPassphraseLen: 8
    readonly property bool orientationTransitionRunning: parentPage && parentPage.orientationTransitionRunning
    readonly property bool isLandscape: parentPage && parentPage.isLandscape
    readonly property bool canGenerate: inputField.text.length >= minPassphraseLen && !generating
    readonly property bool generating: foilModel.foilState === FoilAuthModel.FoilGeneratingKey
    readonly property bool landscapeLayout: isLandscape && Screen.sizeCategory < Screen.Large

    function generateKey() {
        if (canGenerate) {
            var dialog = pageStack.push(Qt.resolvedUrl("ConfirmPasswordPage.qml"), {
                password: inputField.text
            })
            dialog.passwordConfirmed.connect(function() {
                foilModel.generateKey(keySize.value, inputField.text)
                pageStack.pop(mainPage)
            })
        }
    }

    HarbourHighlightIcon {
        y: (panel.y > height) ? Math.floor((panel.y - height)/2) :
            (promptLabel.opacity > 0) ? (panel.y - height) :
            Math.floor((panel.y + keySize.y - height)/2)
        width: Theme.itemSizeHuge
        sourceSize.width: width
        source: "images/key.svg"
        anchors.horizontalCenter: parent.horizontalCenter

        // Hide it when it's only partially visible (i.e. in langscape)
        // or getting too close to the edge of the screen
        visible: y >= Theme.paddingLarge && !orientationTransitionRunning
    }

    Item {
        id: panel

        width: parent.width
        height: Math.max(button.y + button.height, inputField.y + inputField.height) + (landscapeLayout ? 0 : Theme.paddingLarge)
        // Try to keep the input field centered on the screen
        y: (parent.height > height) ? Math.max(Math.floor(parent.height/2) - inputField.y, 0) : (parent.height - height)

        InfoLabel {
            id: promptLabel

            //: Label text
            //% "You need to generate the key and select the password before you can encrypt your authentication tokens"
            text: qsTrId("foilauth-generate-label-key_needed")
            opacity: (parent.y >= Theme.paddingLarge && !orientationTransitionRunning) ? 1 : 0
        }

        ComboBox {
            id: keySize

            //: Combo box label
            //% "Key size"
            label: qsTrId("foilauth-generate-label-key_size")
            enabled: !generating
            width: parent.width
            anchors {
                top: promptLabel.bottom
                topMargin: Theme.paddingLarge
            }
            menu: ContextMenu {
                MenuItem { text: "1024" }
                MenuItem { text: "1500" }
                MenuItem { text: "2048" }
            }
            Component.onCompleted: currentIndex = 2 // default
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
            EnterKey.enabled: canGenerate
            EnterKey.onClicked: generateKey()
        }

        Button {
            id: button

            anchors.topMargin: Theme.paddingLarge
            text: generating ?
                //: Button label
                //% "Generating..."
                qsTrId("foilauth-generate-button-generating_key") :
                //: Button label
                //% "Generate key"
                qsTrId("foilauth-generate-button-generate_key")
            enabled: canGenerate
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
                    anchors.rightMargin: 0
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
                    anchors.rightMargin: Theme.horizontalPageMargin
                }
            ]
        }
    ]
}
