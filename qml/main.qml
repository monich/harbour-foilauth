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

    Connections {
        target: HarbourSystemState
        onLockModeChanged: {
            if (target.locked) {
                FoilAuthModel.lock(false);
            }
        }
    }
}
