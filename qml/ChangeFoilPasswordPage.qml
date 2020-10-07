import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

import "harbour"

Page {
    id: page

    allowedOrientations: window.allowedOrientations

    property Page mainPage
    property bool wrongPassword
    property alias currentPassword: currentPasswordField.text
    property alias newPassword: newPasswordField.text

    readonly property var foilModel: FoilAuthModel
    readonly property bool landscapeLayout: isLandscape && Screen.sizeCategory < Screen.Large
    readonly property real screenHeight: isPortrait ? Screen.height : Screen.width
    readonly property bool canChangePassword: currentPassword.length > 0 && newPassword.length > 0 &&
                            currentPassword !== newPassword && !wrongPassword

    function invalidPassword() {
        wrongPassword = true
        wrongPasswordAnimation.start()
        currentPasswordField.requestFocus()
    }

    function changePassword() {
        if (canChangePassword) {
            if (foilModel.checkPassword(currentPassword)) {
                pageStack.push(Qt.resolvedUrl("ConfirmPasswordPage.qml"), {
                    password: newPassword
                }).passwordConfirmed.connect(function() {
                    if (foilModel.changePassword(currentPassword, newPassword)) {
                        pageStack.pop(mainPage)
                    } else {
                        invalidPassword()
                    }
                })
            } else {
                invalidPassword()
            }
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Activating) {
            currentPasswordField.requestFocus()
        }
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height
        y: Math.min((parent.height - panel.height)/2,
            parent.height - (changePasswordButton.y + changePasswordButton.height + Theme.paddingMedium))

        InfoLabel {
            id: prompt

            //: Password change prompt
            //% "Please enter the current and the new password"
            text: qsTrId("foilauth-change_password_page-label-enter_passwords")

            // Hide it when it's only partially visible
            opacity: (panel.y < 0) ? 0 : 1
            Behavior on opacity { FadeAnimation {} }
        }

        HarbourPasswordInputField {
            id: currentPasswordField

            anchors {
                left: panel.left
                top: prompt.bottom
                topMargin: Theme.paddingLarge
                bottomMargin: Theme.paddingLarge
            }

            //: Placeholder and label for the current password prompt
            //% "Current password"
            label: qsTrId("foilauth-change_password_page-text_field_label-current_password")
            placeholderText: label
            onTextChanged: page.wrongPassword = false
            EnterKey.enabled: text.length > 0
            EnterKey.onClicked: newPasswordField.focus = true
        }

        HarbourPasswordInputField {
            id: newPasswordField

            anchors {
                left: currentPasswordField.left
                right: currentPasswordField.right
                top: currentPasswordField.bottom
            }

            //: Placeholder and label for the new password prompt
            //% "New password"
            placeholderText: qsTrId("foilauth-change_password_page-text_field_label-new_password")
            label: placeholderText
            EnterKey.enabled: page.canChangePassword
            EnterKey.onClicked: page.changePassword()
        }

        Button {
            id: changePasswordButton

            //: Button label
            //% "Change password"
            text: qsTrId("foilauth-change_password_page-button-change_password")
            enabled: page.canChangePassword
            onClicked: page.changePassword()
        }
    }

    Loader {
        anchors {
            top: parent.top
            topMargin: screenHeight - height - Theme.paddingLarge
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
        }
        opacity: (FoilAuthSettings.sharedKeyWarning2 && FoilAuth.otherFoilAppsInstalled) ? 1 : 0
        active: opacity > 0
        sourceComponent: Component {
            FoilAppsWarning {
                onClicked: FoilAuthSettings.sharedKeyWarning2 = false
            }
        }
        Behavior on opacity { FadeAnimation {} }
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
                    target: currentPasswordField
                    anchors.right: panel.right
                },
                PropertyChanges {
                    target: currentPasswordField
                    anchors {
                        rightMargin: 0
                        bottomMargin: Theme.paddingLarge
                    }
                },
                AnchorChanges {
                    target: changePasswordButton
                    anchors {
                        top: newPasswordField.bottom
                        right: undefined
                        horizontalCenter: parent.horizontalCenter
                        bottom: undefined
                    }
                },
                PropertyChanges {
                    target: changePasswordButton
                    anchors.rightMargin: 0
                }
            ]
        },
        State {
            name: "landscape"
            when: landscapeLayout
            changes: [
                AnchorChanges {
                    target: currentPasswordField
                    anchors.right: changePasswordButton.left
                },
                PropertyChanges {
                    target: currentPasswordField
                    anchors {
                        rightMargin: Theme.horizontalPageMargin
                        bottomMargin: Theme.paddingSmall
                    }
                },
                AnchorChanges {
                    target: changePasswordButton
                    anchors {
                        top: undefined
                        right: panel.right
                        horizontalCenter: undefined
                        bottom: newPasswordField.verticalCenter
                    }
                },
                PropertyChanges {
                    target: changePasswordButton
                    anchors.rightMargin: Theme.horizontalPageMargin
                }
            ]
        }
    ]
}
