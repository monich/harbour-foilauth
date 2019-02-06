import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Page {
    id: page

    allowedOrientations: window.allowedOrientations
    forwardNavigation: false
    property string password
    property bool wrongPassword
    readonly property bool landscapeLayout: appLandscapeMode && Screen.sizeCategory < Screen.Large

    signal passwordConfirmed()

    function checkPassword() {
        if (inputField.text === password) {
            page.passwordConfirmed()
        } else {
            wrongPassword = true
            wrongPasswordAnimation.start()
            inputField.requestFocus()
        }
    }

    function canCheckPassword() {
        return inputField.text.length > 0 && inputField.text.length > 0 && !wrongPassword
    }

    onStatusChanged: {
        if (status === PageStatus.Activating) {
            inputField.requestFocus()
        }
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height
        y: (parent.height > height) ? Math.floor((parent.height - height)/2) : (parent.height - height)

        InfoLabel {
            id: title

            //: Password confirmation label
            //% "Please type in your new password one more time"
            text: qsTrId("foilnotes-confirm_password_page-info_label")
        }

        Label {
            id: description

            //: Password confirmation description
            //% "Make sure you don't forget your password. It's impossible to either recover it or to access the encrypted notes without knowing it. Better take it seriously."
            text: qsTrId("foilnotes-confirm_password_page-description")
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
            anchors {
                top: title.bottom
                topMargin: Theme.paddingLarge
            }
        }

        HarbourPasswordInputField {
            id: inputField

            anchors {
                left: parent.left
                top: description.bottom
                topMargin: Theme.paddingLarge
            }
            //: Placeholder for the password confirmation prompt
            //% "New password again"
            placeholderText: qsTrId("foilnotes-confirm_password_page-text_field_placeholder-new_password")
            //: Label for the password confirmation prompt
            //% "New password"
            label: qsTrId("foilnotes-confirm_password_page-text_field_label-new_password")
            onTextChanged: page.wrongPassword = false
            EnterKey.onClicked: page.checkPassword()
        }

        Button {
            id: button

            anchors.horizontalCenter: parent.horizontalCenter
            //: Button label (confirm password)
            //% "Confirm"
            text: qsTrId("foilnotes-confirm_password_page-button-confirm")
            enabled: page.canCheckPassword()
            onClicked: page.checkPassword()
        }
    }

    HarbourShakeAnimation  {
        id: wrongPasswordAnimation

        target: panel
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
                        top: description.bottom
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
