import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

ApplicationWindow {
    id: appWindow

    allowedOrientations: appAllowedOrientations
    initialPage: Component { MainPage { } }
    cover: Component {  CoverPage { } }

    readonly property int appAllowedOrientations: Orientation.All
    readonly property bool appLandscapeMode: orientation === Qt.LandscapeOrientation ||
        orientation === Qt.InvertedLandscapeOrientation

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
            if (target.locked) {
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
}
