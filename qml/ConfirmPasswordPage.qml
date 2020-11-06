import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Dialog {
    id: dialog

    allowedOrientations: window.allowedOrientations
    forwardNavigation: false

    property string password
    property bool wrongPassword

    readonly property bool landscapeLayout: isLandscape && Screen.sizeCategory < Screen.Large
    readonly property bool canCheckPassword: inputField.text.length > 0 &&
                                             inputField.text.length > 0 && !wrongPassword

    signal passwordConfirmed()

    function checkPassword() {
        if (inputField.text === password) {
            dialog.passwordConfirmed()
        } else {
            wrongPassword = true
            wrongPasswordAnimation.start()
            inputField.requestFocus()
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Activating) {
            inputField.requestFocus()
        }
    }

    // Otherwise width is changing with a delay, causing visible layout changes
    onIsLandscapeChanged: width = isLandscape ? Screen.height : Screen.width

    Item {
        id: panel

        width: parent.width
        height: Math.max(button.y + button.height, inputField.y + inputField.height) + (landscapeLayout ? 0 : Theme.paddingLarge)
        // Try to keep the input field centered on the screen
        y: (parent.height > height) ? Math.max(Math.floor(parent.height/2) - inputField.y, 0) : (parent.height - height)

        InfoLabel {
            id: prompt

            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x

            //: Password confirmation label
            //% "Please type in your new password one more time"
            text: qsTrId("foilnotes-confirm_password_page-info_label")

            // Hide it when it's only partially visible
            opacity: (parent.y < Theme.paddingLarge) ? 0 : 1
        }

        Label {
            id: warning

            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            anchors {
                top: prompt.bottom
                topMargin: Theme.paddingLarge
            }

            //: Password confirmation description
            //% "Make sure you don't forget your password. It's impossible to either recover it or to access the encrypted notes without knowing it. Better take it seriously."
            text: qsTrId("foilnotes-confirm_password_page-description")
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
        }

        HarbourPasswordInputField {
            id: inputField

            anchors {
                left: parent.left
                top: warning.bottom
                topMargin: Theme.paddingLarge
            }

            //: Placeholder for the password confirmation prompt
            //% "New password again"
            placeholderText: qsTrId("foilnotes-confirm_password_page-text_field_placeholder-new_password")
            //: Label for the password confirmation prompt
            //% "New password"
            label: qsTrId("foilnotes-confirm_password_page-text_field_label-new_password")
            onTextChanged: dialog.wrongPassword = false
            EnterKey.enabled: dialog.canCheckPassword
            EnterKey.onClicked: dialog.checkPassword()
        }

        Button {
            id: button

            anchors.topMargin: Theme.paddingLarge
            //: Button label (confirm password)
            //% "Confirm"
            text: qsTrId("foilnotes-confirm_password_page-button-confirm")
            enabled: dialog.canCheckPassword
            onClicked: dialog.checkPassword()
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
                    anchors {
                        rightMargin: 0
                        bottomMargin: Theme.paddingLarge
                    }
                },
                AnchorChanges {
                    target: button
                    anchors {
                        top: inputField.bottom
                        right: undefined
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
                    anchors {
                        rightMargin: Theme.horizontalPageMargin
                        bottomMargin: Theme.paddingSmall
                    }
                },
                AnchorChanges {
                    target: button
                    anchors {
                        top: warning.bottom
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
