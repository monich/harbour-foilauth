import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

ApplicationWindow {
    id: appWindow

    allowedOrientations: Orientation.All
    initialPage: HarbourProcessState.jailedApp ? jailPageComponent : mainPageComponent
    cover: Component {  CoverPage { } }

    Component {
        id: mainPageComponent

        MainPage {
            allowedOrientations: appWindow.allowedOrientations
        }
    }

    Component {
        id: jailPageComponent

        JailPage {
            allowedOrientations: appWindow.allowedOrientations
        }
    }

    Timer {
        id: lockTimer

        interval: FoilAuthSettings.autoLockTime
        onTriggered: FoilAuthModel.lock(true);
    }

    Connections {
        target: HarbourSystemState

        property bool wasDimmed

        onDisplayStatusChanged: {
            if (target.displayStatus === HarbourSystemState.MCE_DISPLAY_DIM) {
                wasDimmed = true
            } else if (target.displayStatus === HarbourSystemState.MCE_DISPLAY_ON) {
                wasDimmed = false
            }
        }
        onLockedChanged: {
            lockTimer.stop()
            if (FoilAuthSettings.autoLock && target.locked) {
                if (wasDimmed) {
                    // Give the user some time to wake up the screen and
                    // avoid re-entering the password.
                    lockTimer.start()
                } else {
                    FoilAuthModel.lock(false);
                }
            }
        }
    }

    Connections {
        target: FoilAuthSettings

        onAutoLockChanged: {
            lockTimer.stop()
            // It's so unlikely that settings change when the device is locked
            // But it's possible!
            if (target.autoLock && HarbourSystemState.locked) {
                FoilAuthModel.lock(false);
            }
        }
    }
}
