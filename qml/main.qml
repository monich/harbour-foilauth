import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilauth 1.0

ApplicationWindow {
    id: appWindow

    allowedOrientations: Orientation.All
    initialPage: HarbourProcessState.jailedApp ? jailPageComponent : mainPageComponent
    cover: Component {  CoverPage { } }

    function resetAutoLock() {
        lockTimer.stop()
        if (FoilAuthModel.foilState === FoilAuthModel.FoilModelReady &&
            FoilAuthSettings.autoLock && HarbourSystemState.locked) {
            lockTimer.start()
        }
    }

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

    HarbourWakeupTimer {
        id: lockTimer

        interval: FoilAuthSettings.autoLockTime
        onTriggered: FoilAuthModel.lock(false);
    }

    Connections {
        target: HarbourSystemState
        onLockedChanged: resetAutoLock()
    }

    Connections {
        target: FoilAuthSettings
        onAutoLockChanged: resetAutoLock()
    }

    Connections {
        target: FoilAuthModel
        onFoilStateChanged: resetAutoLock()
    }
}
