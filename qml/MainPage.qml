import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0
import org.nemomobile.notifications 1.0

import "foil-ui"
import "harbour"

Page {
    id: thisPage

    allowedOrientations: appAllowedOrientations

    readonly property var foilModel: FoilAuthModel
    property var foilUi

    // Otherwise width may change with a delay, causing visible layout changes
    onIsLandscapeChanged: width = isLandscape ? Screen.height : Screen.width

    function getFoilUi() {
        if (!foilUi) {
            foilUi = foilUiComponent.createObject(thisPage)
        }
        return foilUi
    }

    Component {
        id: foilUiComponent

        QtObject {
            readonly property real opacityFaint: HarbourTheme.opacityFaint
            readonly property real opacityLow: HarbourTheme.opacityLow
            readonly property real opacityHigh: HarbourTheme.opacityHigh
            readonly property real opacityOverlay: HarbourTheme.opacityOverlay

            readonly property var settings: FoilAuthSettings
            readonly property bool otherFoilAppsInstalled: FoilAuth.otherFoilAppsInstalled
            function isLockedState(foilState) {
                return foilState === FoilAuthModel.FoilLocked ||
                    foilState === FoilAuthModel.FoilLockedTimedOut
            }
            function isReadyState(foilState) {
                return foilState === FoilAuthModel.FoilModelReady
            }
            function isGeneratingKeyState(foilState) {
                return foilState === FoilAuthModel.FoilGeneratingKey
            }
            function qsTrEnterPasswordViewMenuGenerateNewKey() {
                //: Pulley menu item
                //% "Generate a new key"
                return qsTrId("foilauth-menu-generate_key")
            }
            function qsTrEnterPasswordViewEnterPasswordLong() {
                //: Password prompt label (long)
                //% "Secret tokens are locked. Please enter your password"
                return qsTrId("foilauth-foil_password-label-enter_password_long")
            }
            function qsTrEnterPasswordViewEnterPasswordShort() {
                //: Password prompt label (short)
                //% "Secret tokens are locked"
                return qsTrId("foilauth-foil_password-label-enter_password_short")
            }
            function qsTrEnterPasswordViewButtonUnlock() {
                //: Button label
                //% "Unlock"
                return qsTrId("foilauth-foil_password-button-unlock")
            }
            function qsTrEnterPasswordViewButtonUnlocking() {
                //: Button label
                //% "Unlocking..."
                return qsTrId("foilauth-foil_password-button-unlocking")
            }
            function qsTrAppsWarningText() {
                //: Warning text, small size label below the password prompt
                //% "Note that all Foil apps use the same encryption key and password."
                return qsTrId("foilauth-foil_apps_warning")
            }
            function qsTrGenerateKeyWarningTitle() {
                //: Title for the new key warning
                //% "Warning"
                return qsTrId("foilauth-generate_key_warning-title")
            }
            function qsTrGenerateKeyWarningText() {
                //: Warning shown prior to generating the new key
                //% "Once you have generated a new key, you are going to lose access to all the files encrypted by the old key. Note that the same key is used by all Foil apps, such as Foil Notes and Foil Pics. If you have forgotten your password, then keep in mind that most likely it's computationally easier to brute-force your password and recover the old key than to decrypt files for which the key is lost."
                return qsTrId("foilauth-generate_key_warning-text")
            }
            function qsTrGenerateNewKeyPrompt() {
                //: Label text
                //% "You are about to generate a new key"
                return qsTrId("foilauth-generate_key-title")
            }
            function qsTrGenerateKeySizeLabel() {
                //: Combo box label
                //% "Key size"
                return qsTrId("foilauth-generate-label-key_size")
            }
            function qsTrGenerateKeyPasswordDescription(minLen) {
                //: Password field label
                //% "Type at least %0 character(s)"
                return qsTrId("foilauth-generate-label-minimum_length",minLen).arg(minLen)
            }
            function qsTrGenerateKeyButtonGenerate() {
                //: Button label
                //% "Generate key"
                return qsTrId("foilauth-generate-button-generate_key")
            }
            function qsTrGenerateKeyButtonGenerating() {
                //: Button label
                //% "Generating..."
                return qsTrId("foilauth-generate-button-generating_key")
            }
            function qsTrConfirmPasswordPrompt() {
                //: Password confirmation label
                //% "Please type in your new password one more time"
                return qsTrId("foilauth-confirm_password-info_label")
            }
            function qsTrConfirmPasswordDescription() {
                //: Password confirmation description
                //% "Make sure you don't forget your password. It's impossible to either recover it or to access the encrypted tokens without knowing it. Better take it seriously."
                return qsTrId("foilauth-confirm_password-description")
            }
            function qsTrConfirmPasswordRepeatPlaceholder() {
                //: Placeholder for the password confirmation prompt
                //% "New password again"
                return qsTrId("foilauth-confirm_password-placeholder-new_password")
            }
            function qsTrConfirmPasswordRepeatLabel() {
                //: Label for the password confirmation prompt
                //% "New password"
                return qsTrId("foilauth-confirm_password-label-new_password")
            }
            function qsTrConfirmPasswordButton() {
                //: Button label (confirm password)
                //% "Confirm"
                return qsTrId("foilauth-confirm_password-button")
            }
        }
    }

    Connections {
        target: foilModel
        property int lastFoilState
        onFoilStateChanged: {
            // Don't let the progress screens disappear too fast
            switch (target.foilState) {
            case FoilAuthModel.FoilGeneratingKey:
                generatingKeyTimer.start()
                break
            case FoilAuthModel.FoilDecrypting:
                decryptingTimer.start()
                break
            case FoilAuthModel.FoilLocked:
            case FoilAuthModel.FoilLockedTimedOut:
                pageStack.pop(thisPage, PageStackAction.Immediate)
                break
            }
            lastFoilState = target.foilState
        }
        onKeyGenerated: {
            //: Pop-up notification
            //% "Generated new key"
            notification.previewBody = qsTrId("foilauth-notification-generated_key")
            notification.publish()
        }
        onPasswordChanged: {
            //: Pop-up notification
            //% "Password changed"
            notification.previewBody = qsTrId("foilauth-notification-password_changed")
            notification.publish()
        }
    }

    Timer {
        id: generatingKeyTimer

        interval: 1000
    }

    Timer {
        id: decryptingTimer

        interval: 1000
    }

    Notification {
        id: notification

        expireTimeout: 2000
        Component.onCompleted: {
            if ('icon' in notification) {
                notification.icon = "icon-s-certificates"
            }
        }
    }

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: height
        // GenerateFoilKeyView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilAuthModel.FoilKeyMissing) ? 1 : 0
            sourceComponent: Component {
                FoilUiGenerateKeyView {
                    //: Label text
                    //% "You need to generate the key and select the password before you can encrypt your authentication tokens"
                    prompt: qsTrId("foilauth-generate-label-key_needed")
                    page: thisPage
                    foilUi: getFoilUi()
                    foilModel: thisPage.foilModel
                }
            }
            Behavior on opacity { FadeAnimation { } }
        }

        // GeneratingFoilKeyView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilAuthModel.FoilGeneratingKey ||
                generatingKeyTimer.running) ? 1 : 0
            sourceComponent: Component {
                FoilUiGeneratingKeyView {
                    //: Progress view label
                    //% "Generating new key..."
                    text: qsTrId("foilauth-generating-label")
                    isLandscape: thisPage.isLandscape
                }
            }
            Behavior on opacity { FadeAnimation { } }
        }

        // EnterFoilPasswordView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilAuthModel.FoilLocked ||
                        foilModel.foilState === FoilAuthModel.FoilLockedTimedOut) ? 1 : 0
            sourceComponent: Component {
                FoilUiEnterPasswordView {
                    foilUi: getFoilUi()
                    foilModel: thisPage.foilModel
                    page: thisPage
                    iconComponent: Component {
                        Image {
                            width: Theme.itemSizeHuge
                            height: width
                            sourceSize.width: width
                            source: "images/foilauth.svg"
                        }
                    }
                }
            }
            Behavior on opacity { FadeAnimation { } }
        }

        // DecryptingView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilAuthModel.FoilDecrypting ||
                        decryptingTimer.running) ? 1 : 0
            sourceComponent: Component {
                DecryptingView {
                    mainPage: thisPage
                    foilModel: thisPage.foilModel
                }
            }
            Behavior on opacity { FadeAnimation {} }
        }

        // TokenListView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            readonly property bool ready: foilModel.foilState === FoilAuthModel.FoilModelReady
            opacity: (ready && !generatingKeyTimer.running && !decryptingTimer.running) ? 1 : 0
            sourceComponent: Component {
                TokenListView {
                    mainPage: thisPage
                    foilUi: getFoilUi()
                    foilModel: thisPage.foilModel
                }
            }
            Behavior on opacity { FadeAnimation {} }
        }
    }
}
