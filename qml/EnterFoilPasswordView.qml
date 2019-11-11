import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

import "harbour"

SilicaFlickable {
    id: view

    property Page mainPage
    property bool wrongPassword
    readonly property var foilModel: FoilAuthModel
    readonly property bool landscapeLayout: appLandscapeMode && Screen.sizeCategory < Screen.Large
    readonly property bool unlocking: foilModel.foilState !== FoilAuthModel.FoilLocked &&
                                    foilModel.foilState !== FoilAuthModel.FoilLockedTimedOut

    function enterPassword() {
        if (!foilModel.unlock(inputField.text)) {
            wrongPassword = true
            wrongPasswordAnimation.start()
            inputField.requestFocus()
        }
    }

    PullDownMenu {
        id: pullDownMenu

        MenuItem {
            //: Pulley menu item
            //% "Generate a new key"
            text: qsTrId("foilauth-menu-generate_key")
            onClicked: {
                var warning = pageStack.push(Qt.resolvedUrl("GenerateFoilKeyWarning.qml"));
                warning.accepted.connect(function() {
                    // Replace the warning page with a slide. This may
                    // occasionally generate "Warning: cannot pop while
                    // transition is in progress" if the user taps the
                    // page stack indicator (as opposed to the Accept
                    // button) but this warning is fairly harmless:
                    //
                    // _dialogDone (Dialog.qml:124)
                    // on_NavigationChanged (Dialog.qml:177)
                    // navigateForward (PageStack.qml:291)
                    // onClicked (private/PageStackIndicator.qml:174)
                    //
                    warning.canNavigateForward = false
                    pageStack.replace(Qt.resolvedUrl("GenerateFoilKeyPage.qml"), {
                        mainPage: view.mainPage
                    })
                })
            }
        }
    }

    Image {
        width: Theme.itemSizeHuge
        sourceSize.width: width
        source: "images/foilauth.svg"
        anchors.horizontalCenter: parent.horizontalCenter
        y: (panel.y > height) ? Math.floor((panel.y - height)/2) : (panel.y - height)
        visible: opacity > 0
        opacity: (y < 0) ? 0 : 1
        Behavior on opacity { FadeAnimation { duration: landscapeLayout ? 0 : 100 } }
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height
        anchors.verticalCenter: parent.verticalCenter

        readonly property bool showLongPrompt: y >= 0

        InfoLabel {
            id: longPrompt

            height: implicitHeight
            visible: panel.showLongPrompt
            //: Password prompt label (long)
            //% "Secret tokens are locked. Please enter your password"
            text: qsTrId("foilauth-foil_password-label-enter_password_long")
        }

        InfoLabel {
            height: implicitHeight
            anchors.bottom: longPrompt.bottom
            visible: !panel.showLongPrompt
            //: Password prompt label (short)
            //% "Secret tokens are locked"
            text: qsTrId("foilauth-foil_password-label-enter_password_short")
        }

        HarbourPasswordInputField {
            id: inputField

            anchors {
                left: panel.left
                top: longPrompt.bottom
                topMargin: Theme.paddingLarge
            }
            enabled: !unlocking
            EnterKey.onClicked: view.enterPassword()
            onTextChanged: view.wrongPassword = false
        }

        Button {
            id: button

            text: unlocking ?
                //: Button label
                //% "Unlocking..."
                qsTrId("foilauth-foil_password-button-unlocking") :
                //: Button label
                //% "Unlock"
                qsTrId("foilauth-foil_password-button-unlock")
            enabled: inputField.text.length > 0 && !unlocking && !wrongPasswordAnimation.running && !wrongPassword
            onClicked: view.enterPassword()
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
        active: FoilAuthSettings.sharedKeyWarning && FoilAuth.otherFoilAppsInstalled
        sourceComponent: Component {
            FoilAppsWarning {
                onClicked: FoilAuthSettings.sharedKeyWarning = false
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
                        top: longPrompt.bottom
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
