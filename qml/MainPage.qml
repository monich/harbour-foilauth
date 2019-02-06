import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0
import org.nemomobile.notifications 1.0

import "harbour"

Page {
    id: page

    allowedOrientations: appAllowedOrientations

    readonly property var foilModel: FoilAuthModel
    readonly property real screenHeight: isPortrait ? Screen.height : Screen.width

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
                pageStack.pop(page, PageStackAction.Immediate)
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

    Notification {
        id: notification

        expireTimeout: 2000
        Component.onCompleted: {
            if ('icon' in notification) {
                notification.icon = "icon-s-certificates"
            }
        }
    }

    // GenerateFoilKeyView
    Loader {
        anchors.fill: parent
        active: opacity > 0
        opacity: (foilModel.foilState === FoilAuthModel.FoilKeyMissing) ? 1 : 0
        sourceComponent: Component { GenerateFoilKeyView { mainPage: page } }
        Behavior on opacity { FadeAnimation { } }
    }

    // GeneratingFoilKeyView
    Loader {
        anchors.fill: parent
        active: opacity > 0
        opacity: (foilModel.foilState === FoilAuthModel.FoilGeneratingKey ||
                    generatingKeyTimer.running) ? 1 : 0
        sourceComponent: Component { GeneratingFoilKeyView { } }
        Behavior on opacity { FadeAnimation { } }
    }

    // EnterFoilPasswordView
    Loader {
        anchors.fill: parent
        active: opacity > 0
        opacity: (foilModel.foilState === FoilAuthModel.FoilLocked ||
                    foilModel.foilState === FoilAuthModel.FoilLockedTimedOut) ? 1 : 0
        sourceComponent: Component { EnterFoilPasswordView { mainPage: page } }
        Behavior on opacity { FadeAnimation { } }
    }

    // DecryptingView
    Loader {
        anchors.fill: parent
        active: opacity > 0
        opacity: (foilModel.foilState === FoilAuthModel.FoilDecrypting ||
                    decryptingTimer.running) ? 1 : 0
        sourceComponent: Component { DecryptingView { } }
        Behavior on opacity { FadeAnimation {} }
    }

    // TokenListView
    Loader {
        anchors.fill: parent
        active: opacity > 0
        readonly property bool ready: foilModel.foilState === FoilAuthModel.FoilModelReady
        opacity: (ready && !generatingKeyTimer.running && !decryptingTimer.running) ? 1 : 0
        sourceComponent: Component { TokenListView { mainPage: page } }
        Behavior on opacity { FadeAnimation {} }
    }

    Timer {
        id: generatingKeyTimer

        interval: 1000
    }

    Timer {
        id: decryptingTimer

        interval: 1000
    }
}
